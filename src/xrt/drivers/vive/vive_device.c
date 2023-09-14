// Copyright 2016-2019, Philipp Zabel
// Copyright 2019-2021, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Vive device implementation
 * @author Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * @ingroup drv_vive
 */

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <zlib.h>

#include "util/u_device.h"
#include "util/u_debug.h"
#include "util/u_var.h"
#include "util/u_time.h"
#include "util/u_trace_marker.h"

#include "math/m_api.h"
#include "math/m_predict.h"

#include "os/os_hid.h"
#include "os/os_time.h"

#include "vive.h"
#include "vive_device.h"
#include "vive_protocol.h"
#include "vive_source.h"
#include "xrt/xrt_tracking.h"


static bool
vive_mainboard_power_off(struct vive_device *d);

static inline struct vive_device *
vive_device(struct xrt_device *xdev)
{
	return (struct vive_device *)xdev;
}

static void
vive_device_destroy(struct xrt_device *xdev)
{
	XRT_TRACE_MARKER();

	struct vive_device *d = vive_device(xdev);
	if (d->mainboard_dev)
		vive_mainboard_power_off(d);

	// Destroy the thread object.
	os_thread_helper_destroy(&d->sensors_thread);
	os_thread_helper_destroy(&d->watchman_thread);
	os_thread_helper_destroy(&d->mainboard_thread);

	// Now that the thread is not running we can destroy the lock.

	m_imu_3dof_close(&d->fusion.i3dof);

	os_mutex_destroy(&d->fusion.mutex);

	if (d->mainboard_dev != NULL) {
		os_hid_destroy(d->mainboard_dev);
		d->mainboard_dev = NULL;
	}

	if (d->sensors_dev != NULL) {
		os_hid_destroy(d->sensors_dev);
		d->sensors_dev = NULL;
	}

	if (d->watchman_dev != NULL) {
		os_hid_destroy(d->watchman_dev);
		d->watchman_dev = NULL;
	}

	vive_config_teardown(&d->config);

	m_relation_history_destroy(&d->fusion.relation_hist);

	// Remove the variable tracking.
	u_var_remove_root(d);

	u_device_free(&d->base);
}

static void
vive_device_update_inputs(struct xrt_device *xdev)
{
	XRT_TRACE_MARKER();

	struct vive_device *d = vive_device(xdev);
	VIVE_TRACE(d, "ENTER!");
}

static void
vive_device_get_3dof_tracked_pose(struct xrt_device *xdev,
                                  enum xrt_input_name name,
                                  uint64_t at_timestamp_ns,
                                  struct xrt_space_relation *out_relation)
{
	XRT_TRACE_MARKER();

	struct vive_device *d = vive_device(xdev);

	if (name != XRT_INPUT_GENERIC_HEAD_POSE) {
		U_LOG_E("unknown input name");
		return;
	}

	struct xrt_space_relation relation = {0};
	relation.relation_flags = XRT_SPACE_RELATION_BITMASK_ALL;

	os_mutex_lock(&d->fusion.mutex);
	m_relation_history_get(d->fusion.relation_hist, at_timestamp_ns, &relation);
	os_mutex_unlock(&d->fusion.mutex);

	relation.relation_flags = XRT_SPACE_RELATION_BITMASK_ALL; // Needed after history_get
	relation.pose.position = d->pose.position;
	relation.linear_velocity = (struct xrt_vec3){0, 0, 0};

	*out_relation = relation;
	d->pose = out_relation->pose;
}

//! Specific pose corrections for Basalt and a Valve Index headset
//! @todo Test and fix for other headsets (vive/vivepro)
XRT_MAYBE_UNUSED static inline struct xrt_pose
vive_device_correct_pose_from_basalt(struct xrt_pose pose)
{
	struct xrt_quat q = {-0.70710678, 0, 0, 0.70710678};
	math_quat_rotate(&q, &pose.orientation, &pose.orientation);
	math_quat_rotate_vec3(&q, &pose.position, &pose.position);

	return pose;
}

static void
vive_device_get_slam_tracked_pose(struct xrt_device *xdev,
                                  enum xrt_input_name name,
                                  uint64_t at_timestamp_ns,
                                  struct xrt_space_relation *out_relation)
{
	XRT_TRACE_MARKER();

	struct vive_device *d = vive_device(xdev);
	xrt_tracked_slam_get_tracked_pose(d->tracking.slam, at_timestamp_ns, out_relation);

	int pose_bits = XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT | XRT_SPACE_RELATION_POSITION_TRACKED_BIT;
	bool pose_tracked = out_relation->relation_flags & pose_bits;

	if (pose_tracked) {
#if defined(XRT_HAVE_BASALT_SLAM)
		d->pose = vive_device_correct_pose_from_basalt(out_relation->pose);
#else
		d->pose = out_relation->pose;
#endif
	}

	out_relation->pose = d->pose;
	out_relation->relation_flags = (enum xrt_space_relation_flags)(
	    XRT_SPACE_RELATION_ORIENTATION_VALID_BIT | XRT_SPACE_RELATION_POSITION_VALID_BIT |
	    XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT | XRT_SPACE_RELATION_POSITION_TRACKED_BIT);
}

static void
vive_device_get_tracked_pose(struct xrt_device *xdev,
                             enum xrt_input_name name,
                             uint64_t at_timestamp_ns,
                             struct xrt_space_relation *out_relation)
{
	XRT_TRACE_MARKER();

	struct vive_device *d = vive_device(xdev);
	if (d->tracking.slam_enabled && d->slam_over_3dof) {
		vive_device_get_slam_tracked_pose(xdev, name, at_timestamp_ns, out_relation);
	} else {
		vive_device_get_3dof_tracked_pose(xdev, name, at_timestamp_ns, out_relation);
	}
	math_pose_transform(&d->offset, &out_relation->pose, &out_relation->pose);
}

static void
vive_device_get_view_poses(struct xrt_device *xdev,
                           const struct xrt_vec3 *default_eye_relation,
                           uint64_t at_timestamp_ns,
                           uint32_t view_count,
                           struct xrt_space_relation *out_head_relation,
                           struct xrt_fov *out_fovs,
                           struct xrt_pose *out_poses)
{
	XRT_TRACE_MARKER();

	// Only supports two views.
	assert(view_count <= 2);

	u_device_get_view_poses(xdev, default_eye_relation, at_timestamp_ns, view_count, out_head_relation, out_fovs,
	                        out_poses);

	// This is for the Index' canted displays, on the Vive [Pro] they are identity.
	struct vive_device *d = vive_device(xdev);
	for (uint32_t i = 0; i < view_count && i < ARRAY_SIZE(d->config.display.rot); i++) {
		out_poses[i].orientation = d->config.display.rot[i];
	}
}

static int
vive_mainboard_get_device_info(struct vive_device *d)
{
	struct vive_headset_mainboard_device_info_report report = {
	    .id = VIVE_HEADSET_MAINBOARD_DEVICE_INFO_REPORT_ID,
	};
	uint16_t edid_vid;
	uint16_t type;
	int ret;

	ret = os_hid_get_feature(d->mainboard_dev, report.id, (uint8_t *)&report, sizeof(report));
	if (ret < 0)
		return ret;

	type = __le16_to_cpu(report.type);
	if (type != VIVE_HEADSET_MAINBOARD_DEVICE_INFO_REPORT_TYPE || report.len != 60) {
		VIVE_WARN(d, "Unexpected device info!");
		return -1;
	}

	edid_vid = __be16_to_cpu(report.edid_vid);

	d->config.firmware.display_firmware_version = __le32_to_cpu(report.display_firmware_version);

	VIVE_INFO(d, "EDID Manufacturer ID: %c%c%c, Product code: 0x%04x", '@' + (edid_vid >> 10),
	          '@' + ((edid_vid >> 5) & 0x1f), '@' + (edid_vid & 0x1f), __le16_to_cpu(report.edid_pid));
	VIVE_INFO(d, "Display firmware version: %u", d->config.firmware.display_firmware_version);

	return 0;
}


static bool
vive_mainboard_power_on(struct vive_device *d)
{
	int ret;
	ret = os_hid_set_feature(d->mainboard_dev, (const uint8_t *)&power_on_report, sizeof(power_on_report));
	VIVE_DEBUG(d, "Power on: %d", ret);
	return true;
}

static bool
vive_mainboard_power_off(struct vive_device *d)
{
	int ret;
	ret = os_hid_set_feature(d->mainboard_dev, (const uint8_t *)&power_off_report, sizeof(power_off_report));
	VIVE_DEBUG(d, "Power off: %d", ret);

	return true;
}

static void
vive_mainboard_decode_message(struct vive_device *d, struct vive_mainboard_status_report *report)
{
	uint16_t ipd;
	uint16_t lens_separation;
	uint16_t proximity;

	if (__le16_to_cpu(report->unknown) != 0x2cd0 || report->len != 60 || report->reserved1 ||
	    report->reserved2[0]) {
		VIVE_WARN(d, "Unexpected message content.");
	}

	ipd = __le16_to_cpu(report->ipd);
	lens_separation = __le16_to_cpu(report->lens_separation);
	proximity = __le16_to_cpu(report->proximity);

	if (d->board.ipd != ipd) {
		d->board.ipd = ipd;
		d->board.lens_separation = lens_separation;
		VIVE_TRACE(d, "IPD %4.1f mm. Lens separation %4.1f mm.", 1e-2 * ipd, 1e-2 * lens_separation);
	}

	if (d->board.proximity != proximity) {
		VIVE_TRACE(d, "Proximity %d", proximity);
		d->board.proximity = proximity;
	}

	if (d->board.button != report->button) {
		d->board.button = report->button;
		VIVE_TRACE(d, "Button %d.", report->button);
	}
}

static inline int
oldest_sequence_index(uint8_t a, uint8_t b, uint8_t c)
{
	if (a == (uint8_t)(b + 2))
		return 1;
	if (b == (uint8_t)(c + 2))
		return 2;

	return 0;
}

static void
update_imu(struct vive_device *d, const void *buffer)
{
	XRT_TRACE_MARKER();

	uint64_t now_ns = os_monotonic_get_ns();

	const struct vive_imu_report *report = buffer;
	const struct vive_imu_sample *sample = report->sample;
	uint8_t last_seq = d->imu.sequence;
	int i;
	int j;

	/*
	 * The three samples are updated round-robin. New messages
	 * can contain already seen samples in any place, but the
	 * sequence numbers should always be consecutive.
	 * Start at the sample with the oldest sequence number.
	 */
	i = oldest_sequence_index(sample[0].seq, sample[1].seq, sample[2].seq);

	/* From there, handle all new samples */
	for (j = 3; j; --j, i = (i + 1) % 3) {
		float scale;
		uint8_t seq;

		sample = report->sample + i;
		seq = sample->seq;

		/* Skip already seen samples */
		if (seq == last_seq || seq == (uint8_t)(last_seq - 1) || seq == (uint8_t)(last_seq - 2)) {
			continue;
		}

		ticks_to_ns(sample->time, &d->imu.last_sample_ticks, &d->imu.last_sample_ts_ns);

		int16_t acc[3] = {
		    (int16_t)__le16_to_cpu(sample->acc[0]),
		    (int16_t)__le16_to_cpu(sample->acc[1]),
		    (int16_t)__le16_to_cpu(sample->acc[2]),
		};

		scale = (float)d->config.imu.acc_range / 32768.0f;
		struct xrt_vec3 acceleration = {
		    scale * d->config.imu.acc_scale.x * acc[0] - d->config.imu.acc_bias.x,
		    scale * d->config.imu.acc_scale.y * acc[1] - d->config.imu.acc_bias.y,
		    scale * d->config.imu.acc_scale.z * acc[2] - d->config.imu.acc_bias.z,
		};

		int16_t gyro[3] = {
		    (int16_t)__le16_to_cpu(sample->gyro[0]),
		    (int16_t)__le16_to_cpu(sample->gyro[1]),
		    (int16_t)__le16_to_cpu(sample->gyro[2]),
		};

		scale = (float)d->config.imu.gyro_range / 32768.0f;
		struct xrt_vec3 angular_velocity = {
		    scale * d->config.imu.gyro_scale.x * gyro[0] - d->config.imu.gyro_bias.x,
		    scale * d->config.imu.gyro_scale.y * gyro[1] - d->config.imu.gyro_bias.y,
		    scale * d->config.imu.gyro_scale.z * gyro[2] - d->config.imu.gyro_bias.z,
		};

		VIVE_TRACE(d, "ACC  %f %f %f", acceleration.x, acceleration.y, acceleration.z);

		VIVE_TRACE(d, "GYRO %f %f %f", angular_velocity.x, angular_velocity.y, angular_velocity.z);

		switch (d->config.variant) {
		case VIVE_VARIANT_VIVE:
			// flip all except x axis
			acceleration.x = +acceleration.x;
			acceleration.y = -acceleration.y;
			acceleration.z = -acceleration.z;

			angular_velocity.x = +angular_velocity.x;
			angular_velocity.y = -angular_velocity.y;
			angular_velocity.z = -angular_velocity.z;
			break;
		case VIVE_VARIANT_PRO:
			// flip all except y axis
			acceleration.x = -acceleration.x;
			acceleration.y = +acceleration.y;
			acceleration.z = -acceleration.z;

			angular_velocity.x = -angular_velocity.x;
			angular_velocity.y = +angular_velocity.y;
			angular_velocity.z = -angular_velocity.z;
			break;
		case VIVE_VARIANT_INDEX: {
			// Flip all axis and re-order.
			struct xrt_vec3 acceleration_fixed;
			acceleration_fixed.x = -acceleration.y;
			acceleration_fixed.y = -acceleration.x;
			acceleration_fixed.z = -acceleration.z;
			acceleration = acceleration_fixed;

			struct xrt_vec3 angular_velocity_fixed;
			angular_velocity_fixed.x = -angular_velocity.y;
			angular_velocity_fixed.y = -angular_velocity.x;
			angular_velocity_fixed.z = -angular_velocity.z;
			angular_velocity = angular_velocity_fixed;
		} break;
		default: VIVE_ERROR(d, "Unhandled Vive variant"); return;
		}

		d->imu.sequence = seq;

		struct xrt_space_relation rel = {0};
		rel.relation_flags =
		    XRT_SPACE_RELATION_ORIENTATION_VALID_BIT | XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT;

		os_mutex_lock(&d->fusion.mutex);
		m_imu_3dof_update(&d->fusion.i3dof, d->imu.last_sample_ts_ns, &acceleration, &angular_velocity);
		rel.pose.orientation = d->fusion.i3dof.rot;
		m_relation_history_push(d->fusion.relation_hist, &rel, now_ns);
		os_mutex_unlock(&d->fusion.mutex);

		vive_source_push_imu_packet(d->source, d->imu.last_sample_ts_ns, acceleration, angular_velocity);
	}
}


/*
 *
 * Mainboard thread
 *
 */

static bool
vive_mainboard_read_one_msg(struct vive_device *d)
{
	uint8_t buffer[64];

	int ret = os_hid_read(d->mainboard_dev, buffer, sizeof(buffer), 1000);
	if (ret == 0) {
		// Time out
		return true;
	}
	if (ret < 0) {
		VIVE_ERROR(d, "Failed to read device '%i'!", ret);
		return false;
	}

	switch (buffer[0]) {
	case VIVE_MAINBOARD_STATUS_REPORT_ID:
		if (ret != sizeof(struct vive_mainboard_status_report)) {
			VIVE_ERROR(d, "Mainboard status report has invalid size.");
			return false;
		}
		vive_mainboard_decode_message(d, (struct vive_mainboard_status_report *)buffer);
		break;
	default: VIVE_ERROR(d, "Unknown mainboard message type %d", buffer[0]); break;
	}

	return true;
}

static void *
vive_mainboard_run_thread(void *ptr)
{
	struct vive_device *d = (struct vive_device *)ptr;

	os_thread_helper_lock(&d->mainboard_thread);
	while (os_thread_helper_is_running_locked(&d->mainboard_thread)) {
		os_thread_helper_unlock(&d->mainboard_thread);

		if (!vive_mainboard_read_one_msg(d)) {
			return NULL;
		}

		// Just keep swimming.
		os_thread_helper_lock(&d->mainboard_thread);
	}

	return NULL;
}


/*
 *
 * Sensor thread.
 *
 */

static int
vive_sensors_enable_watchman(struct vive_device *d, bool enable_sensors)
{
	uint8_t buf[5] = {0x04};
	int ret;

	/* Enable vsync timestamps, enable/disable sensor reports */
	buf[1] = enable_sensors ? 0x00 : 0x01;
	ret = os_hid_set_feature(d->sensors_dev, buf, sizeof(buf));
	if (ret < 0)
		return ret;

	/*
	 * Reset Lighthouse Rx registers? Without this, inactive channels are
	 * not cleared to 0xff.
	 */
	buf[0] = 0x07;
	buf[1] = 0x02;
	return os_hid_set_feature(d->sensors_dev, buf, sizeof(buf));
}

static void
_print_v1_pulse(struct vive_device *d, uint8_t sensor_id, uint32_t timestamp, uint16_t duration)
{
	VIVE_TRACE(d, "[sensor %02d] timestamp %8u ticks (%3.5fs) duration: %d", sensor_id, timestamp,
	           timestamp / (float)VIVE_CLOCK_FREQ, duration);
}

static void
_decode_pulse_report(struct vive_device *d, const void *buffer)
{
	XRT_TRACE_MARKER();

	const struct vive_headset_lighthouse_pulse_report *report = buffer;
	unsigned int i;

	/* The pulses may appear in arbitrary order */
	for (i = 0; i < 9; i++) {
		const struct vive_headset_lighthouse_pulse *pulse;
		uint8_t sensor_id;
		uint16_t duration;
		uint32_t timestamp;

		pulse = &report->pulse[i];

		sensor_id = pulse->id;
		if (sensor_id == 0xff)
			continue;

		timestamp = __le32_to_cpu(pulse->timestamp);
		if (sensor_id == 0xfe) {
			/* TODO: handle vsync timestamp */
			continue;
		}

		if (sensor_id == 0xfd) { // Camera frame timestamp
			vive_source_push_frame_ticks(d->source, pulse->timestamp);
			continue;
		}

		if (sensor_id == 0xfb) {
			/* TODO: Only turns on when the camera is running but not every frame. It
			 * seems to come with every 16h frame on an Index (~3.37hz) */
			continue;
		}

		if (sensor_id > 31) {
			VIVE_ERROR(d, "Unexpected sensor id: %04x", sensor_id);
			return;
		}

		duration = __le16_to_cpu(pulse->duration);

		_print_v1_pulse(d, sensor_id, timestamp, duration);

		lighthouse_watchman_handle_pulse(&d->watchman, sensor_id, duration, timestamp);
	}
}

static const char *
_sensors_get_report_string(uint32_t report_id)
{
	switch (report_id) {
	case VIVE_IMU_REPORT_ID: return "VIVE_IMU_REPORT_ID";
	case VIVE_HEADSET_LIGHTHOUSE_PULSE_REPORT_ID: return "VIVE_HEADSET_LIGHTHOUSE_PULSE_REPORT_ID";
	case VIVE_CONTROLLER_LIGHTHOUSE_PULSE_REPORT_ID: return "VIVE_CONTROLLER_LIGHTHOUSE_PULSE_REPORT_ID";
	case VIVE_HEADSET_LIGHTHOUSE_V2_PULSE_REPORT_ID: return "VIVE_HEADSET_LIGHTHOUSE_V2_PULSE_REPORT_ID";
	default: return "Unknown";
	}
}

static bool
_is_report_size_valid(struct vive_device *d, int size, int report_size, int report_id)
{
	if (size != report_size) {
		VIVE_WARN(d, "Wrong size %d for report %s (%02x). Expected %d.", size,
		          _sensors_get_report_string(report_id), report_id, report_size);
		return false;
	}
	return true;
}

static bool
vive_sensors_read_one_msg(struct vive_device *d,
                          struct os_hid_device *dev,
                          uint32_t report_id,
                          int report_size,
                          void (*process_cb)(struct vive_device *d, const void *buffer))
{
	uint8_t buffer[64];

	int ret = os_hid_read(dev, buffer, sizeof(buffer), 1000);
	if (ret == 0) {
		VIVE_ERROR(d, "Device %p timeout.", (void *)dev);
		// Time out
		return true;
	}
	if (ret < 0) {
		VIVE_ERROR(d, "Failed to read device %p: %i.", (void *)dev, ret);
		return false;
	}

	if (buffer[0] == report_id) {
		if (!_is_report_size_valid(d, ret, report_size, report_id))
			return false;

		process_cb(d, buffer);

	} else {
		VIVE_ERROR(d, "Unexpected sensor report type %s (0x%x).", _sensors_get_report_string(buffer[0]),
		           buffer[0]);
		VIVE_ERROR(d, "Expected %s (0x%x).", _sensors_get_report_string(report_id), report_id);
	}

	return true;
}

static void
_print_v2_pulse(
    struct vive_device *d, uint8_t sensor_id, uint8_t flag, uint32_t timestamp, uint32_t data, uint32_t mask)
{
	char data_str[32];

	for (int k = 0; k < 32; k++) {
		uint8_t idx = 32 - k - 1;
		bool d = (data >> (idx)) & 1u;
		bool m = (mask >> (idx)) & 1u;
		if (m)
			sprintf(&data_str[k], "%d", d);
		else
			sprintf(&data_str[k], "_");
	}

	VIVE_TRACE(d,
	           "[sensor %02d] flag: %03u "
	           "timestamp %8u ticks (%3.5fs) data: %s",
	           sensor_id, flag, timestamp, timestamp / (float)VIVE_CLOCK_FREQ, data_str);
}

static bool
_print_pulse_report_v2(struct vive_device *d, const void *buffer)
{
	XRT_TRACE_MARKER();

	const struct vive_headset_lighthouse_v2_pulse_report *report = buffer;

	for (uint32_t i = 0; i < 4; i++) {
		const struct vive_headset_lighthouse_v2_pulse *p = &report->pulse[i];

		if (p->sensor_id == 0xff)
			continue;

		uint8_t sensor_id = p->sensor_id & 0x7fu;
		if (sensor_id > 31) {
			VIVE_ERROR(d, "Unexpected sensor id: %2u\n", sensor_id);
			return false;
		}

		uint8_t flag = p->sensor_id & 0x80u;
		if (flag != 0x80u && flag != 0) {
			VIVE_WARN(d, "Unexpected flag: %02x\n", flag);
			return false;
		}

		uint32_t timestamp = __le32_to_cpu(p->timestamp);
		_print_v2_pulse(d, sensor_id, flag, timestamp, p->data, p->mask);
	}

	return true;
}

static bool
vive_sensors_read_lighthouse_msg(struct vive_device *d)
{
	uint8_t buffer[64];

	int ret = os_hid_read(d->watchman_dev, buffer, sizeof(buffer), 1000);
	if (ret == 0) {
		// basestations not present/powered off
		VIVE_TRACE(d, "Watchman device timed out.");
		return true;
	}
	if (ret < 0) {
		VIVE_ERROR(d, "Failed to read Watchman device: %i.", ret);
		return false;
	}
	if (ret > 64) {
		VIVE_ERROR(d,
		           "Buffer too big from Watchman device: %i."
		           " Max size is 64",
		           ret);
		return false;
	}

	int expected; // size;

	switch (buffer[0]) {
	case VIVE_HEADSET_LIGHTHOUSE_PULSE_REPORT_ID:
		expected = sizeof(struct vive_headset_lighthouse_pulse_report);
		if (!_is_report_size_valid(d, ret, expected, buffer[0]))
			return false;
		_decode_pulse_report(d, buffer);
		break;
	case VIVE_CONTROLLER_LIGHTHOUSE_PULSE_REPORT_ID:
		expected = sizeof(struct vive_controller_report1);
		// Vive pro gives unexpected size here with V2.
		_is_report_size_valid(d, ret, expected, buffer[0]);
		break;
	case VIVE_HEADSET_LIGHTHOUSE_V2_PULSE_REPORT_ID:
		if (!_is_report_size_valid(d, ret, 59, buffer[0]))
			return false;
		if (!_print_pulse_report_v2(d, buffer))
			return false;
		break;
	default:
		VIVE_ERROR(d, "Unexpected sensor report type %s (0x%x). %d bytes.",
		           _sensors_get_report_string(buffer[0]), buffer[0], ret);
	}

	return true;
}

static void *
vive_watchman_run_thread(void *ptr)
{
	struct vive_device *d = (struct vive_device *)ptr;

	os_thread_helper_lock(&d->watchman_thread);
	while (os_thread_helper_is_running_locked(&d->watchman_thread)) {
		os_thread_helper_unlock(&d->watchman_thread);

		if (d->watchman_dev)
			if (!vive_sensors_read_lighthouse_msg(d))
				return NULL;

		// Just keep swimming.
		os_thread_helper_lock(&d->watchman_thread);
	}

	return NULL;
}

static void *
vive_sensors_run_thread(void *ptr)
{
	struct vive_device *d = (struct vive_device *)ptr;

	os_thread_helper_lock(&d->sensors_thread);
	while (os_thread_helper_is_running_locked(&d->sensors_thread)) {
		os_thread_helper_unlock(&d->sensors_thread);

		if (!vive_sensors_read_one_msg(d, d->sensors_dev, VIVE_IMU_REPORT_ID, 52, update_imu)) {
			return NULL;
		}

		// Just keep swimming.
		os_thread_helper_lock(&d->sensors_thread);
	}

	return NULL;
}

static void
vive_device_switch_hmd_tracker(void *d_ptr)
{
	DRV_TRACE_MARKER();

	struct vive_device *d = (struct vive_device *)d_ptr;
	d->slam_over_3dof = !d->slam_over_3dof;
	struct u_var_button *btn = &d->gui.switch_tracker_btn;

	if (d->slam_over_3dof) { // Use SLAM
		snprintf(btn->label, sizeof(btn->label), "Switch to 3DoF Tracking");
	} else { // Use 3DoF
		snprintf(btn->label, sizeof(btn->label), "Switch to SLAM Tracking");
		os_mutex_lock(&d->fusion.mutex);
		m_imu_3dof_reset(&d->fusion.i3dof);
		d->fusion.i3dof.rot = d->pose.orientation;
		os_mutex_unlock(&d->fusion.mutex);
	}
}

static void
vive_device_setup_ui(struct vive_device *d)
{
	u_var_add_root(d, "Vive Device", true);

	u_var_add_gui_header(d, NULL, "Tracking");
	if (d->tracking.slam_enabled) {
		d->gui.switch_tracker_btn.cb = vive_device_switch_hmd_tracker;
		d->gui.switch_tracker_btn.ptr = d;
		u_var_add_button(d, &d->gui.switch_tracker_btn, "Switch to 3DoF Tracking");
	}
	u_var_add_pose(d, &d->pose, "Tracked Pose");
	u_var_add_pose(d, &d->offset, "Pose Offset");

	u_var_add_gui_header(d, &d->gui.fusion, "3DoF Tracking");
	m_imu_3dof_add_vars(&d->fusion.i3dof, d, "");
	u_var_add_gui_header(d, &d->gui.calibration, "Calibration");
	u_var_add_vec3_f32(d, &d->config.imu.acc_scale, "acc_scale");
	u_var_add_vec3_f32(d, &d->config.imu.acc_bias, "acc_bias");
	u_var_add_vec3_f32(d, &d->config.imu.gyro_scale, "gyro_scale");
	u_var_add_vec3_f32(d, &d->config.imu.gyro_bias, "gyro_bias");

	u_var_add_gui_header(d, NULL, "SLAM Tracking");
	u_var_add_ro_text(d, d->gui.slam_status, "Tracker status");

	u_var_add_gui_header(d, NULL, "Hand Tracking");
	u_var_add_ro_text(d, d->gui.hand_status, "Tracker status");
}

static bool
compute_distortion(struct xrt_device *xdev, int view, float u, float v, struct xrt_uv_triplet *result)
{
	XRT_TRACE_MARKER();

	struct vive_device *d = vive_device(xdev);
	return u_compute_distortion_vive(&d->config.distortion[view], u, v, result);
}

void
vive_set_trackers_status(struct vive_device *d, struct vive_tracking_status status)
{
	bool dof3_enabled = true; // We always have at least 3dof HMD tracking
	bool slam_wanted = status.slam_wanted;
	bool slam_supported = status.slam_supported;
	bool slam_enabled = status.slam_enabled;
	bool hand_wanted = status.hand_wanted;
	bool hand_supported = status.hand_supported;
	bool hand_enabled = status.hand_enabled;

	d->base.orientation_tracking_supported = dof3_enabled || slam_enabled;
	d->base.position_tracking_supported = slam_enabled;
	d->base.hand_tracking_supported = false; // this is handled by a separate hand device
	d->base.device_type = XRT_DEVICE_TYPE_HMD;

	d->tracking.slam_enabled = slam_enabled;
	d->tracking.hand_enabled = hand_enabled;

	d->slam_over_3dof = slam_enabled; // We prefer SLAM over 3dof tracking if possible

	const char *slam_status = d->tracking.slam_enabled ? "Enabled"
	                          : !slam_wanted           ? "Disabled by the user (envvar set to false)"
	                          : !slam_supported        ? "Unavailable (not built)"
	                                                   : "Failed to initialize";

	const char *hand_status = d->tracking.hand_enabled ? "Enabled"
	                          : !hand_wanted           ? "Disabled by the user (envvar set to false)"
	                          : !hand_supported        ? "Unavailable (not built)"
	                                                   : "Failed to initialize";

	snprintf(d->gui.slam_status, sizeof(d->gui.slam_status), "%s", slam_status);
	snprintf(d->gui.hand_status, sizeof(d->gui.hand_status), "%s", hand_status);
}

struct vive_device *
vive_device_create(struct os_hid_device *mainboard_dev,
                   struct os_hid_device *sensors_dev,
                   struct os_hid_device *watchman_dev,
                   enum VIVE_VARIANT variant,
                   struct vive_tracking_status tstatus,
                   struct vive_source *vs)
{
	XRT_TRACE_MARKER();

	enum u_device_alloc_flags flags =
	    (enum u_device_alloc_flags)(U_DEVICE_ALLOC_HMD | U_DEVICE_ALLOC_TRACKING_NONE);
	struct vive_device *d = U_DEVICE_ALLOCATE(struct vive_device, flags, 1, 0);

	m_relation_history_create(&d->fusion.relation_hist);

	size_t idx = 0;
	d->base.hmd->blend_modes[idx++] = XRT_BLEND_MODE_OPAQUE;
	d->base.hmd->blend_mode_count = idx;

	d->base.update_inputs = vive_device_update_inputs;
	d->base.get_tracked_pose = vive_device_get_tracked_pose;
	d->base.get_view_poses = vive_device_get_view_poses;
	d->base.destroy = vive_device_destroy;
	d->base.inputs[0].name = XRT_INPUT_GENERIC_HEAD_POSE;
	d->base.name = XRT_DEVICE_GENERIC_HMD;
	d->mainboard_dev = mainboard_dev;
	d->sensors_dev = sensors_dev;
	d->log_level = debug_get_log_option_vive_log();
	d->watchman_dev = watchman_dev;

	d->base.hmd->distortion.models = XRT_DISTORTION_MODEL_COMPUTE;
	d->base.hmd->distortion.preferred = XRT_DISTORTION_MODEL_COMPUTE;
	d->base.compute_distortion = compute_distortion;

	if (d->mainboard_dev) {
		vive_mainboard_power_on(d);
		vive_mainboard_get_device_info(d);
	}
	vive_read_firmware(d->sensors_dev, &d->config.firmware.firmware_version, &d->config.firmware.hardware_revision,
	                   &d->config.firmware.hardware_version_micro, &d->config.firmware.hardware_version_minor,
	                   &d->config.firmware.hardware_version_major);

	/*
	VIVE_INFO(d, "Firmware version %u %s@%s FPGA %u.%u",
	          d->firmware.firmware_version, report.string1, report.string2,
	          report.fpga_version_major, report.fpga_version_minor);
	*/

	VIVE_INFO(d, "Firmware version %u", d->config.firmware.firmware_version);
	VIVE_INFO(d, "Hardware revision: %d rev %d.%d.%d", d->config.firmware.hardware_revision,
	          d->config.firmware.hardware_version_major, d->config.firmware.hardware_version_minor,
	          d->config.firmware.hardware_version_micro);

	vive_get_imu_range_report(d->sensors_dev, &d->config.imu.gyro_range, &d->config.imu.acc_range);
	VIVE_INFO(d, "Vive gyroscope range     %f", d->config.imu.gyro_range);
	VIVE_INFO(d, "Vive accelerometer range %f", d->config.imu.acc_range);

	char *config = vive_read_config(d->sensors_dev);

	d->config.log_level = d->log_level;
	// usb connected HMD variant is known because of USB id, config parsing relies on it.
	if (config != NULL) {
		vive_config_parse(&d->config, config, d->log_level);
		free(config);
	}

	// TODO: Replace hard coded values from OpenHMD with config
	double w_meters = 0.122822 / 2.0;
	double h_meters = 0.068234;
	double lens_horizontal_separation = 0.057863;
	double eye_to_screen_distance = 0.023226876441867737;

	uint32_t w_pixels = d->config.display.eye_target_width_in_pixels;
	uint32_t h_pixels = d->config.display.eye_target_height_in_pixels;

	// Main display.
	d->base.hmd->screens[0].w_pixels = (int)w_pixels * 2;
	d->base.hmd->screens[0].h_pixels = (int)h_pixels;

	if (d->config.variant == VIVE_VARIANT_INDEX) {
		lens_horizontal_separation = 0.06;
		h_meters = 0.07;
		// eye relief knob adjusts this around [0.0255(near)-0.275(far)]
		eye_to_screen_distance = 0.0255;

		d->base.hmd->screens[0].nominal_frame_interval_ns = (uint64_t)time_s_to_ns(1.0f / 144.0f);
	} else {
		d->base.hmd->screens[0].nominal_frame_interval_ns = (uint64_t)time_s_to_ns(1.0f / 90.0f);
	}

	double fov = 2 * atan2(w_meters - lens_horizontal_separation / 2.0, eye_to_screen_distance);

	struct xrt_vec2 lens_center[2];

	for (uint8_t eye = 0; eye < 2; eye++) {
		struct xrt_view *v = &d->base.hmd->views[eye];
		v->display.w_pixels = w_pixels;
		v->display.h_pixels = h_pixels;
		v->viewport.w_pixels = w_pixels;
		v->viewport.h_pixels = h_pixels;
		v->viewport.y_pixels = 0;
		lens_center[eye].y = (float)h_meters / 2.0f;
		v->rot = u_device_rotation_ident;
	}

	// Left
	lens_center[0].x = (float)(w_meters - lens_horizontal_separation / 2.0);
	d->base.hmd->views[0].viewport.x_pixels = 0;

	// Right
	lens_center[1].x = (float)lens_horizontal_separation / 2.0f;
	d->base.hmd->views[1].viewport.x_pixels = w_pixels;

	for (uint8_t eye = 0; eye < 2; eye++) {
		if (!math_compute_fovs(w_meters, (double)lens_center[eye].x, fov, h_meters, (double)lens_center[eye].y,
		                       0, &d->base.hmd->distortion.fov[eye])) {
			VIVE_ERROR(d, "Failed to compute the partial fields of view.");
			free(d);
			return NULL;
		}
	}

	// Init threads.
	os_thread_helper_init(&d->mainboard_thread);
	os_thread_helper_init(&d->sensors_thread);
	os_thread_helper_init(&d->watchman_thread);

	d->source = vs;
	d->pose = (struct xrt_pose)XRT_POSE_IDENTITY;
	d->offset = (struct xrt_pose)XRT_POSE_IDENTITY;

	int ret;

	if (watchman_dev != NULL) {
		ret = vive_sensors_enable_watchman(d, true);
		if (ret < 0) {
			VIVE_ERROR(d, "Could not enable watchman receiver.");
		} else {
			lighthouse_watchman_init(&d->watchman, "headset");
			VIVE_DEBUG(d, "Successfully enabled watchman receiver.");
		}
	}

	if (d->mainboard_dev) {
		ret = os_thread_helper_start(&d->mainboard_thread, vive_mainboard_run_thread, d);
		if (ret != 0) {
			VIVE_ERROR(d, "Failed to start mainboard thread!");
			vive_device_destroy((struct xrt_device *)d);
			return NULL;
		}
	}

	switch (d->config.variant) {
	case VIVE_VARIANT_VIVE: snprintf(d->base.str, XRT_DEVICE_NAME_LEN, "HTC Vive (vive)"); break;
	case VIVE_VARIANT_PRO: snprintf(d->base.str, XRT_DEVICE_NAME_LEN, "HTC Vive Pro (vive)"); break;
	case VIVE_VARIANT_INDEX: snprintf(d->base.str, XRT_DEVICE_NAME_LEN, "Valve Index (vive)"); break;
	case VIVE_UNKNOWN: snprintf(d->base.str, XRT_DEVICE_NAME_LEN, "Unknown HMD (vive)"); break;
	}
	snprintf(d->base.serial, XRT_DEVICE_NAME_LEN, "%s", d->config.firmware.device_serial_number);

	vive_set_trackers_status(d, tstatus);

	// Initialize 3DoF tracker
	m_imu_3dof_init(&d->fusion.i3dof, M_IMU_3DOF_USE_GRAVITY_DUR_20MS);

	ret = os_mutex_init(&d->fusion.mutex);
	if (ret != 0) {
		VIVE_ERROR(d, "Failed to init 3dof mutex");
		return false;
	}

	ret = os_thread_helper_start(&d->sensors_thread, vive_sensors_run_thread, d);
	if (ret != 0) {
		VIVE_ERROR(d, "Failed to start sensors thread!");
		vive_device_destroy((struct xrt_device *)d);
		return NULL;
	}

	ret = os_thread_helper_start(&d->watchman_thread, vive_watchman_run_thread, d);
	if (ret != 0) {
		VIVE_ERROR(d, "Failed to start watchman thread!");
		vive_device_destroy((struct xrt_device *)d);
		return NULL;
	}

	vive_device_setup_ui(d);

	return d;
}
