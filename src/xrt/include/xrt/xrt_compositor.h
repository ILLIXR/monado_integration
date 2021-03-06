// Copyright 2019, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header defining a XRT graphics provider.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup xrt_iface
 */

#pragma once

#include "xrt/xrt_defines.h"

#ifdef __cplusplus
extern "C" {
#endif


/*!
 * Max swapchain images, artificial limit.
 *
 * @ingroup xrt_iface
 */
#define XRT_MAX_SWAPCHAIN_IMAGES 8

/*!
 * Max formats supported by a compositor, artificial limit.
 *
 * @ingroup xrt_iface
 */
#define XRT_MAX_SWAPCHAIN_FORMATS 8

/*!
 * Special flags for creating swapchain images.
 *
 * @ingroup xrt_iface
 */
enum xrt_swapchain_create_flags
{
	XRT_SWAPCHAIN_CREATE_STATIC_IMAGE = (1 << 0),
};

/*!
 * Usage of the swapchain images.
 *
 * @ingroup xrt_iface
 */
enum xrt_swapchain_usage_bits
{
	XRT_SWAPCHAIN_USAGE_COLOR = 0x00000001,
	XRT_SWAPCHAIN_USAGE_DEPTH_STENCIL = 0x00000002,
	XRT_SWAPCHAIN_USAGE_UNORDERED_ACCESS = 0x00000004,
	XRT_SWAPCHAIN_USAGE_TRANSFER_SRC = 0x00000008,
	XRT_SWAPCHAIN_USAGE_TRANSFER_DST = 0x00000010,
	XRT_SWAPCHAIN_USAGE_SAMPLED = 0x00000020,
	XRT_SWAPCHAIN_USAGE_MUTABLE_FORMAT = 0x00000040
};

/*!
 * View type to be rendered to by the compositor.
 *
 * @ingroup xrt_iface
 */
enum xrt_view_type
{
	XRT_VIEW_TYPE_MONO = 1,
	XRT_VIEW_TYPE_STEREO = 2,
};

/*!
 * Common swapchain base.
 *
 * @ingroup xrt_iface
 */
struct xrt_swapchain
{
	/*!
	 * Number of images, the images themselves are on the subclasses.
	 */
	uint32_t num_images;

	/*!
	 * Number of array layers per image.
	 */
	uint32_t array_size;

	/*!
	 * Must have called release_image before calling this function.
	 */
	void (*destroy)(struct xrt_swapchain *sc);

	/*!
	 * See xrWaitSwapchainImage, must make sure that no image is acquired
	 * before calling acquire_image.
	 */
	bool (*acquire_image)(struct xrt_swapchain *xc, uint32_t *index);

	/*!
	 * See xrWaitSwapchainImage, state tracker needs to track index.
	 */
	bool (*wait_image)(struct xrt_swapchain *xc,
	                   uint64_t timeout,
	                   uint32_t index);

	/*!
	 * See xrReleaseSwapchainImage, state tracker needs to track index.
	 */
	bool (*release_image)(struct xrt_swapchain *xc, uint32_t index);
};

/*!
 * Common compositor base.
 *
 * @ingroup xrt_iface
 */
struct xrt_compositor
{
	/*!
	 * Number of formats.
	 */
	uint32_t num_formats;

	/*!
	 * Supported formats.
	 */
	int64_t formats[XRT_MAX_SWAPCHAIN_FORMATS];

	/*!
	 * Create a swapchain with a set of images.
	 */
	struct xrt_swapchain *(*create_swapchain)(
	    struct xrt_compositor *xc,
	    enum xrt_swapchain_create_flags create,
	    enum xrt_swapchain_usage_bits bits,
	    int64_t format,
	    uint32_t sample_count,
	    uint32_t width,
	    uint32_t height,
	    uint32_t face_count,
	    uint32_t array_size,
	    uint32_t mip_count);

	/*!
	 * Poll events from this compositor.
	 *
	 * This function is very much WIP.
	 */
	void (*poll_events)(struct xrt_compositor *xc, uint64_t *WIP);

	/*!
	 * This function is implicit in the OpenXR spec but made explicit here.
	 */
	void (*prepare_session)(struct xrt_compositor *xc);

	/*!
	 * See xrBeginSession.
	 */
	void (*begin_session)(struct xrt_compositor *xc,
	                      enum xrt_view_type view_type);

	/*!
	 * See xrEndSession, unlike the OpenXR one the state tracker is
	 * responsible to call discard frame before calling this function. See
	 * discard_frame.
	 */
	void (*end_session)(struct xrt_compositor *xc);

	/*!
	 * See xrWaitFrame.
	 */
	void (*wait_frame)(struct xrt_compositor *xc,
	                   int64_t *predicted_display_time,
	                   int64_t *predicted_display_period);

	/*!
	 * See xrBeginFrame.
	 */
	void (*begin_frame)(struct xrt_compositor *xc);

	/*!
	 * This isn't in the OpenXR API but is explicit in the XRT interfaces.
	 *
	 * Two calls to xrBeginFrame will cause the state tracker to call.
	 *
	 * ```c
	 * xc->begin_frame(xc)
	 * xc->discard_frame(xc)
	 * xc->begin_frame(xc)
	 * ```
	 */
	void (*discard_frame)(struct xrt_compositor *xc);

	/*!
	 * See xrEndFrame.
	 */
	void (*end_frame)(struct xrt_compositor *xc,
	                  enum xrt_blend_mode blend_mode,
	                  struct xrt_swapchain **xscs,
	                  const uint32_t *image_index,
	                  uint32_t *layers,
	                  uint32_t num_swapchains);

	/*!
	 * Teardown the compositor.
	 *
	 * The state tracker must have made sure that no frames or sessions are
	 * currently pending. See discard_frame, end_frame, end_session.
	 */
	void (*destroy)(struct xrt_compositor *xc);
};

/*!
 * Helper for xrt_compositor::create_swapchain
 *
 * @ingroup xrt_iface
 */
XRT_MAYBE_UNUSED static struct xrt_swapchain *
xrt_comp_create_swapchain(struct xrt_compositor *xc,
                          enum xrt_swapchain_create_flags create,
                          enum xrt_swapchain_usage_bits bits,
                          int64_t format,
                          uint32_t sample_count,
                          uint32_t width,
                          uint32_t height,
                          uint32_t face_count,
                          uint32_t array_size,
                          uint32_t mip_count)
{
	return xc->create_swapchain(xc, create, bits, format, sample_count,
	                            width, height, face_count, array_size,
	                            mip_count);
}

/*!
 * Helper for xrt_compositor::poll_events
 *
 * @ingroup xrt_iface
 */
XRT_MAYBE_UNUSED static void
xrt_comp_poll_events(struct xrt_compositor *xc, uint64_t *WIP)
{
	xc->poll_events(xc, WIP);
}

/*!
 * Helper for xrt_compositor::prepare_session
 *
 * @ingroup xrt_iface
 */
XRT_MAYBE_UNUSED static void
xrt_comp_prepare_session(struct xrt_compositor *xc)
{
	xc->prepare_session(xc);
}

/*!
 * Helper for xrt_compositor::begin_session
 *
 * @ingroup xrt_iface
 */
XRT_MAYBE_UNUSED static void
xrt_comp_begin_session(struct xrt_compositor *xc, enum xrt_view_type view_type)
{
	xc->begin_session(xc, view_type);
}

/*!
 * Helper for xrt_compositor::end_session
 *
 * @ingroup xrt_iface
 */
XRT_MAYBE_UNUSED static void
xrt_comp_end_session(struct xrt_compositor *xc)
{
	xc->end_session(xc);
}

/*!
 * Helper for xrt_compositor::wait_frame
 *
 * @ingroup xrt_iface
 */
XRT_MAYBE_UNUSED static void
xrt_comp_wait_frame(struct xrt_compositor *xc,
                    int64_t *predicted_display_time,
                    int64_t *predicted_display_period)
{
	xc->wait_frame(xc, predicted_display_time, predicted_display_period);
}

/*!
 * Helper for xrt_compositor::begin_frame
 *
 * @ingroup xrt_iface
 */
XRT_MAYBE_UNUSED static void
xrt_comp_begin_frame(struct xrt_compositor *xc)
{
	xc->begin_frame(xc);
}

/*!
 * Helper for xrt_compositor::discard_frame
 *
 * @ingroup xrt_iface
 */
XRT_MAYBE_UNUSED static void
xrt_comp_discard_frame(struct xrt_compositor *xc)
{
	xc->discard_frame(xc);
}

/*!
 * Helper for xrt_compositor::end_frame
 *
 * @ingroup xrt_iface
 */
XRT_MAYBE_UNUSED static void
xrt_comp_end_frame(struct xrt_compositor *xc,
                   enum xrt_blend_mode blend_mode,
                   struct xrt_swapchain **xscs,
                   const uint32_t *image_index,
                   uint32_t *layers,
                   uint32_t num_swapchains)
{
	xc->end_frame(xc, blend_mode, xscs, image_index, layers,
	              num_swapchains);
}

/*!
 * Helper for xrt_compositor::destroy, does a null check and sets xc_ptr to
 * null if freed.
 *
 * @ingroup xrt_iface
 */
XRT_MAYBE_UNUSED static void
xrt_comp_destroy(struct xrt_compositor **xc_ptr)
{
	struct xrt_compositor *xc = *xc_ptr;
	if (xc == NULL) {
		return;
	}

	xc->destroy(xc);
	*xc_ptr = NULL;
}


/*
 *
 * OpenGL interface.
 *
 */

/*!
 * @ingroup xrt_iface comp_client
 */
struct xrt_swapchain_gl
{
	struct xrt_swapchain base;

	// GLuint
	unsigned int images[XRT_MAX_SWAPCHAIN_IMAGES];
	// GLuint
	unsigned int memory[XRT_MAX_SWAPCHAIN_IMAGES];
};

/*!
 * @ingroup xrt_iface comp_client
 */
struct xrt_compositor_gl
{
	struct xrt_compositor base;
};

static inline struct xrt_swapchain_gl *
xrt_swapchain_gl(struct xrt_swapchain *xsc)
{
	return (struct xrt_swapchain_gl *)xsc;
}

static inline struct xrt_compositor_gl *
xrt_compositor_gl(struct xrt_compositor *xc)
{
	return (struct xrt_compositor_gl *)xc;
}


/*
 *
 * Vulkan interface.
 *
 */

typedef struct VkImage_T *VkImage;
typedef struct VkDeviceMemory_T *VkDeviceMemory;

/*!
 * Base class for a Vulkan client swapchain.
 *
 * @ingroup xrt_iface comp_client
 */
struct xrt_swapchain_vk
{
	struct xrt_swapchain base;

	VkImage images[XRT_MAX_SWAPCHAIN_IMAGES];
	VkDeviceMemory mems[XRT_MAX_SWAPCHAIN_IMAGES];
};

/*!
 * Base class for a Vulkan client compositor.
 *
 * @ingroup xrt_iface comp_client
 */
struct xrt_compositor_vk
{
	struct xrt_compositor base;
};

static inline struct xrt_swapchain_vk *
xrt_swapchain_vk(struct xrt_swapchain *xsc)
{
	return (struct xrt_swapchain_vk *)xsc;
}

static inline struct xrt_compositor_vk *
xrt_compositor_vk(struct xrt_compositor *xc)
{
	return (struct xrt_compositor_vk *)xc;
}


/*
 *
 * FD interface, aka DMABUF.
 *
 */

/*!
 * A single image of a fd based swapchain.
 *
 * @ingroup xrt_iface comp
 */
struct xrt_image_fd
{
	size_t size;
	int fd;
	int _pad;
};

/*!
 * A swapchain that exposes fd to be imported into a client API.
 *
 * @ingroup xrt_iface comp
 */
struct xrt_swapchain_fd
{
	struct xrt_swapchain base;

	struct xrt_image_fd images[XRT_MAX_SWAPCHAIN_IMAGES];
};

/*!
 * Main compositor.
 *
 * @ingroup xrt_iface comp
 */
struct xrt_compositor_fd
{
	struct xrt_compositor base;
};

static inline struct xrt_swapchain_fd *
xrt_swapchain_fd(struct xrt_swapchain *xsc)
{
	return (struct xrt_swapchain_fd *)xsc;
}

static inline struct xrt_compositor_fd *
xrt_compositor_fd(struct xrt_compositor *xc)
{
	return (struct xrt_compositor_fd *)xc;
}


#ifdef __cplusplus
}
#endif
