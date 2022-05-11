// Copyright 2022, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helpers for system objects like @ref xrt_system_devices.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_results.h"
#include "xrt/xrt_frame.h"
#include "xrt/xrt_system.h"
#include "xrt/xrt_instance.h"


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * Helper struct to manage devices by implementing the @ref xrt_system_devices.
 *
 * The default destroy function that is set by @ref u_system_devices_allocate
 * will first destroy all of the @ref xrt_device and then destroy all nodes
 * in the @ref xrt_frame_context.
 *
 * @ingroup aux_util
 */
struct u_system_devices
{
	struct xrt_system_devices base;

	//! Frame context for visual tracking.
	struct xrt_frame_context xfctx;
};

/*!
 * Small inline helper to cast from @ref xrt_system_devices.
 *
 * @ingroup aux_util
 */
static inline struct u_system_devices *
u_system_devices(struct xrt_system_devices *xsysd)
{
	return (struct u_system_devices *)xsysd;
}

/*!
 * Allocates a empty @ref u_system_devices to be filled in by the caller, only
 * the destroy function is filled in.
 *
 * @ingroup aux_util
 */
struct u_system_devices *
u_system_devices_allocate(void);

/*!
 * Takes a @ref xrt_instance, gets the prober from it and then uses the prober
 * to allocate a filled in @ref u_system_devices.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_system_devices_create_from_prober(struct xrt_instance *xinst, struct xrt_system_devices **out_xsysd);

/*!
 * Destroy an u_system_devices_allocate and owned devices - helper function.
 *
 * @param[in,out] usysd_ptr A pointer to the u_system_devices_allocate struct pointer.
 *
 * Will destroy the system devices if *usysd_ptr is not NULL. Will then set *usysd_ptr to NULL.
 *
 * @public @memberof u_system_devices_allocate
 */
static inline void
u_system_devices_destroy(struct u_system_devices **usysd_ptr)
{
	struct u_system_devices *usysd = *usysd_ptr;
	if (usysd == NULL) {
		return;
	}

	*usysd_ptr = NULL;
	usysd->base.destroy(&usysd->base);
}


#ifdef __cplusplus
}
#endif