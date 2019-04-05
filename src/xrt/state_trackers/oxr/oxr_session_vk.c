// Copyright 2018-2019, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Holds Vulkan specific session functions.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup oxr_main
 * @ingroup comp_client
 */

#include <stdlib.h>

#include "util/u_misc.h"

#include "xrt/xrt_gfx_vk.h"

#include "oxr_objects.h"
#include "oxr_logger.h"
#include "oxr_two_call.h"
#include "oxr_handle.h"


VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vkGetInstanceProcAddr(VkInstance instance, const char *pName);

XrResult
oxr_session_create_vk(struct oxr_logger *log,
                      struct oxr_system *sys,
                      XrGraphicsBindingVulkanKHR *next,
                      struct oxr_session **out_session)
{
	struct xrt_compositor_vk *xcvk = xrt_gfx_vk_provider_create(
	    sys->device, sys->inst->timekeeping, next->instance,
	    vkGetInstanceProcAddr, next->physicalDevice, next->device,
	    next->queueFamilyIndex, next->queueIndex);

	if (xcvk == NULL) {
		return oxr_error(log, XR_ERROR_INITIALIZATION_FAILED,
		                 " failed create a compositor");
	}

	struct oxr_session *sess = NULL;
	XrResult result =
	    OXR_ALLOCATE_HANDLE(log, sess, OXR_XR_DEBUG_SESSION,
	                        oxr_session_destroy, &sys->inst->handle);
	if (result != XR_SUCCESS) {
		xcvk->base.destroy(&xcvk->base);
		return result;
	}
	sess->sys = sys;
	sess->compositor = &xcvk->base;
	sess->create_swapchain = oxr_swapchain_vk_create;

	*out_session = sess;

	return XR_SUCCESS;
}
