// Copyright 2020, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  illixr HMD device.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup drv_illixr
 */


#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "math/m_api.h"
#include "xrt/xrt_device.h"
#include "util/u_var.h"
#include "util/u_misc.h"
#include "util/u_debug.h"
#include "util/u_device.h"
#include "util/u_time.h"
#include "util/u_distortion_mesh.h"

/*
 *
 * Structs and defines.
 *
 */

struct illixr_hmd
{
	struct xrt_device base;

	struct xrt_pose pose;

	bool print_spew;
	bool print_debug;
};


/*
 *
 * Functions
 *
 */

static inline struct illixr_hmd *
illixr_hmd(struct xrt_device *xdev)
{
	return (struct illixr_hmd *)xdev;
}


/*
 * Parses fake input file
 * the file should list desired poses in format:
 * f1, f2, f3, f4\n  // orientation quaternion
 * f1, f2, f3 // position
 */

static const char* test_input = "/tmp/illixr_test_input.txt";

static struct xrt_pose
read_pose_from_file(const char* filename) {
	struct xrt_pose default_pose = {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}};
	struct xrt_pose pose;
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		return default_pose;
	}
	int match = fscanf(file, "%f, %f, %f, %f\n%f, %f, %f", 
		&(pose.orientation.x),
		&(pose.orientation.y),
		&(pose.orientation.z),
		&(pose.orientation.w),
		&(pose.position.x),
		&(pose.position.y),
		&(pose.position.z));
	if (match != 7) {
		fprintf(stderr, "fscanf failed\n");
		fclose(file);
		return default_pose;
	}
	fclose(file);
	return pose;

}

DEBUG_GET_ONCE_BOOL_OPTION(illixr_spew, "illixr_PRINT_SPEW", false)
DEBUG_GET_ONCE_BOOL_OPTION(illixr_debug, "illixr_PRINT_DEBUG", false)

#define DH_SPEW(dh, ...)                                                       \
	do {                                                                   \
		if (dh->print_spew) {                                          \
			fprintf(stderr, "%s - ", __func__);                    \
			fprintf(stderr, __VA_ARGS__);                          \
			fprintf(stderr, "\n");                                 \
		}                                                              \
	} while (false)

#define DH_DEBUG(dh, ...)                                                      \
	do {                                                                   \
		if (dh->print_debug) {                                         \
			fprintf(stderr, "%s - ", __func__);                    \
			fprintf(stderr, __VA_ARGS__);                          \
			fprintf(stderr, "\n");                                 \
		}                                                              \
	} while (false)

#define DH_ERROR(dh, ...)                                                      \
	do {                                                                   \
		fprintf(stderr, "%s - ", __func__);                            \
		fprintf(stderr, __VA_ARGS__);                                  \
		fprintf(stderr, "\n");                                         \
	} while (false)

static void
illixr_hmd_destroy(struct xrt_device *xdev)
{
	struct illixr_hmd *dh = illixr_hmd(xdev);

	// Remove the variable tracking.
	u_var_remove_root(dh);

	u_device_free(&dh->base);
}

static void
illixr_hmd_update_inputs(struct xrt_device *xdev, struct time_state *timekeeping)
{
	// Empty
}

static void
illixr_hmd_get_tracked_pose(struct xrt_device *xdev,
                           enum xrt_input_name name,
                           struct time_state *timekeeping,
                           int64_t *out_timestamp,
                           struct xrt_space_relation *out_relation)
{
	struct illixr_hmd *dh = illixr_hmd(xdev);

	if (name != XRT_INPUT_GENERIC_HEAD_POSE) {
		DH_ERROR(dh, "unknown input name");
		return;
	}

	int64_t now = time_state_get_now(timekeeping);

	*out_timestamp = now;
	// out_relation->pose = dh->pose;
	out_relation->pose = read_pose_from_file(test_input);
	out_relation->relation_flags = (enum xrt_space_relation_flags)(
	    XRT_SPACE_RELATION_ORIENTATION_VALID_BIT |
	    XRT_SPACE_RELATION_POSITION_VALID_BIT);
}

static void
illixr_hmd_get_view_pose(struct xrt_device *xdev,
                        struct xrt_vec3 *eye_relation,
                        uint32_t view_index,
                        struct xrt_pose *out_pose)
{
	struct xrt_pose pose = read_pose_from_file(test_input);

	*out_pose = pose;
}

struct xrt_device *
illixr_hmd_create(void)
{
	enum u_device_alloc_flags flags = (enum u_device_alloc_flags)(
	    U_DEVICE_ALLOC_HMD | U_DEVICE_ALLOC_TRACKING_NONE);
	struct illixr_hmd *dh = U_DEVICE_ALLOCATE(struct illixr_hmd, flags, 1, 0);
	dh->base.update_inputs = illixr_hmd_update_inputs;
	dh->base.get_tracked_pose = illixr_hmd_get_tracked_pose;
	dh->base.get_view_pose = illixr_hmd_get_view_pose;
	dh->base.destroy = illixr_hmd_destroy;
	dh->base.name = XRT_DEVICE_GENERIC_HMD;
	dh->base.hmd->blend_mode = XRT_BLEND_MODE_OPAQUE;
	dh->pose.orientation.w = 1.0f; // All other values set to zero.
	dh->print_spew = debug_get_bool_option_illixr_spew();
	dh->print_debug = debug_get_bool_option_illixr_debug();

	// Print name.
	snprintf(dh->base.str, XRT_DEVICE_NAME_LEN, "Illixr");

	// Setup input.
	dh->base.inputs[0].name = XRT_INPUT_GENERIC_HEAD_POSE;

	// Setup info.
	struct u_device_simple_info info;
	info.display.w_pixels = 1920;
	info.display.h_pixels = 1080;
	info.display.w_meters = 0.13f;
	info.display.h_meters = 0.07f;
	info.lens_horizontal_separation_meters = 0.13f / 2.0f;
	info.lens_vertical_position_meters = 0.07f / 2.0f;
	info.views[0].fov = 85.0f * (M_PI / 180.0f);
	info.views[1].fov = 85.0f * (M_PI / 180.0f);

	if (!u_device_setup_split_side_by_side(&dh->base, &info)) {
		DH_ERROR(dh, "Failed to setup basic device info");
		illixr_hmd_destroy(&dh->base);
		return NULL;
	}

	// Setup variable tracker.
	u_var_add_root(dh, "Illixr", true);
	u_var_add_pose(dh, &dh->pose, "pose");

	if (dh->base.hmd->distortion.preferred == XRT_DISTORTION_MODEL_NONE) {
		// Setup the distortion mesh.
		u_distortion_mesh_none(dh->base.hmd);
	}

	return &dh->base;
}

int
illixr_found(struct xrt_prober *xp,
             struct xrt_prober_device **devices,
             size_t num_devices,
             size_t index,
             struct xrt_device **out_xdevs)
{
	out_xdevs[0] = illixr_hmd_create();
	return 1;
}
