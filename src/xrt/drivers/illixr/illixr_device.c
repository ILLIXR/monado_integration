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
#include <dlfcn.h>
#include <alloca.h>

#include "math/m_api.h"
#include "xrt/xrt_device.h"
#include "util/u_var.h"
#include "util/u_misc.h"
#include "util/u_debug.h"
#include "util/u_device.h"
#include "util/u_time.h"
#include "util/u_distortion_mesh.h"

#include "illixr_component.h"

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

	void *illixr_lib;
	struct illixr_operation_t {
		int (*init)();
		void (*load_component)(const char *path);
		void (*attach_component)(void *f);
		void (*run)(void);
		void (*join)(void);
		void (*destroy)(void);
	} ops;
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
	if (name != XRT_INPUT_GENERIC_HEAD_POSE) {
		DH_ERROR(illixr_hmd(xdev), "unknown input name");
		return;
	}

	int64_t now = time_state_get_now(timekeeping);

	*out_timestamp = now;
	// out_relation->pose = dh->pose;
	out_relation->pose = illixr_read_pose();
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
	struct xrt_pose pose = illixr_read_pose();

	*out_pose = pose;
}

static int
illixr_rt_launch(struct illixr_hmd *dh, const char *path, const char *comp)
{
	// Load library
	if (!(dh->illixr_lib = dlopen(path, RTLD_LAZY|RTLD_LOCAL))) {
		DH_ERROR(dh, "dlopen: %s", dlerror());
		return 1;
	}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
	dh->ops.init = dlsym(dh->illixr_lib, "illixrrt_init");
	dh->ops.load_component = dlsym(dh->illixr_lib, "illixrrt_load_component");
	dh->ops.attach_component = dlsym(dh->illixr_lib, "illixrrt_attach_component");
	dh->ops.run = dlsym(dh->illixr_lib, "illixrrt_run");
	dh->ops.join = dlsym(dh->illixr_lib, "illixrrt_join");
	dh->ops.destroy = dlsym(dh->illixr_lib, "illixrrt_destroy");
#pragma GCC diagnostic pop
	if (!dh->ops.init || !dh->ops.load_component || !dh->ops.attach_component ||
	    !dh->ops.run || !dh->ops.join || !dh->ops.destroy) {
		DH_ERROR(dh, "Missing symbols in IllixrRT library");
		goto dl_cleanup;
	}
	char *libs = strdup(comp);
	if (dh->ops.init() != 0) {
		DH_ERROR(dh, "IllixrRT initialization failed.");
		goto dl_cleanup;
	}

	char *libpath = libs;
	for (size_t i=0; libs[i]; i++) {
		if (libs[i] == ':') {
			libs[i] = '\0';
			dh->ops.load_component(libpath);
			libpath = libs+i+1;
		}
	}
	dh->ops.load_component(libpath);
	dh->ops.attach_component((void *)illixr_monado_create_component);

	dh->ops.run();

	return 0;

dl_cleanup:
	dlclose(dh->illixr_lib);
	return 1;
}

struct xrt_device *
illixr_hmd_create(const char *path, const char *comp)
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

	// Start Illixr Spindle
	if (illixr_rt_launch(dh, path, comp) != 0) {
		DH_ERROR(dh, "Failed to load Illixr Runtime");
		illixr_hmd_destroy(&dh->base);
		return NULL;
	}

	return &dh->base;
}
