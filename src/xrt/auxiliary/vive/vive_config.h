// Copyright 2020-2021, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  vive json header
 * @author Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * @author Moses Turner <moses@collabora.com>
 * @ingroup drv_vive
 */

#pragma once

#include <stdbool.h>

#include "xrt/xrt_defines.h"
#include "util/u_logging.h"
#include "util/u_distortion_mesh.h"
#include "tracking/t_tracking.h"

// public documentation
#define INDEX_MIN_IPD 0.058
#define INDEX_MAX_IPD 0.07

// arbitrary default values
#define DEFAULT_HAPTIC_FREQ 150.0f
#define MIN_HAPTIC_DURATION 0.05f

enum VIVE_VARIANT
{
	VIVE_UNKNOWN = 0,
	VIVE_VARIANT_VIVE,
	VIVE_VARIANT_PRO,
	VIVE_VARIANT_INDEX
};

enum VIVE_CONTROLLER_VARIANT
{
	CONTROLLER_VIVE_WAND,
	CONTROLLER_INDEX_LEFT,
	CONTROLLER_INDEX_RIGHT,
	CONTROLLER_TRACKER_GEN1,
	CONTROLLER_TRACKER_GEN2,
	CONTROLLER_UNKNOWN
};

/*!
 * A calibrated camera on an Index.
 */
struct index_camera
{
	// Note! All the values in this struct are directly pasted in from the JSON values.
	// As such, in my opinion, plus_x, plus_z and position are all "wrong" - all the code I've had to write that
	// uses this struct flips the signs of plus_x, plus_z, and the signs of the X- and Z-components of position.
	// I have no idea why those sign flips were necessary - I suppose Valve/HTC just made some weird decisions when
	// making the config file schemas. I figure it would be very confusing to try to "fix" these values as I'm
	// parsing them, so if you're writing code downstream of this, beware and expect the values in here to be
	// exactly the same as those in the compressed JSON. -Moses
	struct
	{
		struct xrt_vec3 plus_x;
		struct xrt_vec3 plus_z;
		struct xrt_vec3 position; // looks like from head pose
	} extrinsics;

	//! Pose in tracking space.
	struct xrt_pose trackref;

	//! Pose in head space.
	struct xrt_pose headref;

	struct
	{
		double distortion[4]; // Kannala-Brandt
		double center_x;
		double center_y;

		double focal_x;
		double focal_y;
		struct xrt_size image_size_pixels;
	} intrinsics;
};

/*!
 * A single lighthouse senosor point and normal, in IMU space.
 */
struct lh_sensor
{
	struct xrt_vec3 pos;
	uint32_t _pad0;
	struct xrt_vec3 normal;
	uint32_t _pad1;
};

/*!
 * A lighthouse consisting of sensors.
 *
 * All sensors are placed in IMU space.
 */
struct lh_model
{
	struct lh_sensor *sensors;
	size_t sensor_count;
};

struct vive_config
{
	//! log level accessed by the config parser
	enum u_logging_level log_level;

	enum VIVE_VARIANT variant;

	struct
	{
		double acc_range;
		double gyro_range;
		struct xrt_vec3 acc_bias;
		struct xrt_vec3 acc_scale;
		struct xrt_vec3 gyro_bias;
		struct xrt_vec3 gyro_scale;

		//! IMU position in tracking space.
		struct xrt_pose trackref;
	} imu;

	struct
	{
		double lens_separation;
		double persistence;
		int eye_target_height_in_pixels;
		int eye_target_width_in_pixels;

		struct xrt_quat rot[2];

		//! Head position in tracking space.
		struct xrt_pose trackref;
		//! Head position in IMU space.
		struct xrt_pose imuref;
	} display;

	struct
	{
		uint32_t display_firmware_version;
		uint32_t firmware_version;
		uint8_t hardware_revision;
		uint8_t hardware_version_micro;
		uint8_t hardware_version_minor;
		uint8_t hardware_version_major;
		char mb_serial_number[32];
		char model_number[32];
		char device_serial_number[32];
	} firmware;

	struct u_vive_values distortion[2];

	struct
	{
		//! The two cameras.
		struct index_camera view[2];

		//! Left view in right camera space.
		struct xrt_pose left_in_right;

		//! The same but in OpenCV camera space.
		struct xrt_pose opencv;

		//! Have we loaded the config.
		bool valid;
	} cameras;

	struct lh_model lh;
};

struct vive_controller_config
{
	enum u_logging_level log_level;

	enum VIVE_CONTROLLER_VARIANT variant;

	struct
	{
		uint32_t firmware_version;
		uint8_t hardware_revision;
		uint8_t hardware_version_micro;
		uint8_t hardware_version_minor;
		uint8_t hardware_version_major;
		char mb_serial_number[32];
		char model_number[32];
		char device_serial_number[32];
	} firmware;

	struct
	{
		double acc_range;
		double gyro_range;
		struct xrt_vec3 acc_bias;
		struct xrt_vec3 acc_scale;
		struct xrt_vec3 gyro_bias;
		struct xrt_vec3 gyro_scale;

		//! IMU position in tracking space.
		struct xrt_pose trackref;
	} imu;
};

bool
vive_config_parse(struct vive_config *d, char *json_string, enum u_logging_level log_level);

/*!
 * Free any allocated resources on this config.
 */
void
vive_config_teardown(struct vive_config *config);

struct vive_controller_device;

bool
vive_config_parse_controller(struct vive_controller_config *d, char *json_string, enum u_logging_level log_level);

bool
vive_get_stereo_camera_calibration(struct vive_config *d,
                                   struct t_stereo_camera_calibration **calibration_ptr_to_ref,
                                   struct xrt_pose *out_head_in_left_camera);
