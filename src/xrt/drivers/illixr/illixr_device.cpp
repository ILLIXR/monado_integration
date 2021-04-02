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
#include <string>
#include <sstream>

#include "math/m_api.h"
#include "xrt/xrt_device.h"
#include "util/u_var.h"
#include "util/u_misc.h"
#include "util/u_debug.h"
#include "util/u_device.h"
#include "util/u_time.h"
#include "util/u_distortion_mesh.h"

#include "illixr_component.h"
#include "common/dynamic_lib.hpp"
#include "common/runtime.hpp"

#include <GL/glx.h>

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

	// For delaying illixr init from instance creation to session creation
	const char * path;
	const char * comp;
	bool initialized_flag;
	ILLIXR::dynamic_lib* runtime_lib;
	ILLIXR::runtime* runtime;
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

	delete dh->runtime;
	delete dh->runtime_lib;

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

// https://www.fluentcpp.com/2017/04/21/how-to-split-a-string-in-c/
std::vector<std::string> split(const std::string& s, char delimiter) {
   std::vector<std::string> tokens;
   std::string token;
   std::istringstream tokenStream {s};
   while (std::getline(tokenStream, token, delimiter)) {
      tokens.push_back(token);
   }
   return tokens;
}

static int
illixr_rt_launch(struct illixr_hmd *dh, const char *path, const char *comp, void* glctx)
{
	dh->runtime_lib = new ILLIXR::dynamic_lib{ILLIXR::dynamic_lib::create((std::string) path)};
	dh->runtime = dh->runtime_lib->get
		<ILLIXR::runtime*(*)(GLXContext)>("runtime_factory")
		(reinterpret_cast<GLXContext>(glctx));

	for (std::string libpath : split(std::string{comp}, ':')) {
		dh->runtime->load_so(libpath);
	}

	dh->runtime->load_plugin_factory((ILLIXR::plugin_factory)illixr_monado_create_plugin);

	return 0;
}

// Save to delay init till session creation
static void
illixr_hmd_set_output(struct xrt_device *xdev,
	                  enum xrt_output_name name,
	                  struct time_state *timekeeping,
	                  union xrt_output_value *value)
{
	struct illixr_hmd* dh = (struct illixr_hmd*)xdev;
	if (dh->initialized_flag) return;
	// Start Illixr Spindle
	if (illixr_rt_launch(dh, dh->path, dh->comp, (void*)timekeeping) != 0) {
		DH_ERROR(dh, "Failed to load Illixr Runtime");
		illixr_hmd_destroy(&dh->base);
	}
	else {
		dh->initialized_flag = true;
	}
}

extern "C"
struct xrt_device *
illixr_hmd_create(const char *path_in, const char *comp_in)
{
	struct illixr_hmd* dh;
	enum u_device_alloc_flags flags = (enum u_device_alloc_flags)(
	    U_DEVICE_ALLOC_HMD | U_DEVICE_ALLOC_TRACKING_NONE);
	dh = U_DEVICE_ALLOCATE(struct illixr_hmd, flags, 1, 0);
	dh->base.update_inputs = illixr_hmd_update_inputs;
	dh->base.get_tracked_pose = illixr_hmd_get_tracked_pose;
	dh->base.get_view_pose = illixr_hmd_get_view_pose;
	dh->base.set_output = illixr_hmd_set_output;
	dh->base.destroy = illixr_hmd_destroy;
	dh->base.name = XRT_DEVICE_GENERIC_HMD;
	dh->base.hmd->blend_mode = XRT_BLEND_MODE_OPAQUE;
	dh->pose.orientation.w = 1.0f; // All other values set to zero.
	dh->print_spew = debug_get_bool_option_illixr_spew();
	dh->print_debug = debug_get_bool_option_illixr_debug();
	dh->path = path_in;
	dh->comp = comp_in;
	dh->initialized_flag = false;

	// Print name.
	snprintf(dh->base.str, XRT_DEVICE_NAME_LEN, "Illixr");

	// Setup input.
	dh->base.inputs[0].name = XRT_INPUT_GENERIC_HEAD_POSE;

	// Setup info.
	struct u_device_simple_info info;
	info.display.w_pixels = 2048;
	info.display.h_pixels = 1024;
	info.display.w_meters = 0.14f;
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

	// delay init until session create

	return &dh->base;
}
