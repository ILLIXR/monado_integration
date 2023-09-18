# Copyright 2019-2022, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)

add_library(
	aux_vk_gl STATIC
	vk_bundle_init.c
	vk_compositor_flags.c
	vk_documentation.h
	vk_function_loaders.c
	vk_helpers.c
	vk_helpers.h
	vk_image_allocator.c
	vk_image_allocator.h
	vk_image_readback_to_xf_pool.c
	vk_image_readback_to_xf_pool.h
	vk_print.c
	vk_state_creators.c
	vk_sync_objects.c
	vk_time.c
	)
target_link_libraries(aux_vk_gl PUBLIC aux_os_gl aux_util_gl)
target_link_libraries(aux_vk_gl PUBLIC Vulkan::Vulkan Threads::Threads)
target_include_directories(aux_vk_gl PUBLIC ${Vulkan_INCLUDE_DIR})

if(ANDROID)
	target_link_libraries(aux_vk_gl PUBLIC aux_android)
	target_link_libraries(aux_vk_gl PUBLIC ${ANDROID_LIBRARY})
endif()

if(XRT_HAVE_WAYLAND)
	target_include_directories(aux_vk_gl PRIVATE ${WAYLAND_INCLUDE_DIR})
endif()
if(XRT_HAVE_XCB)
	target_include_directories(aux_vk_gl PRIVATE ${XCB_INCLUDE_DIR})
endif()
if(XRT_HAVE_XRANDR)
	target_include_directories(aux_vk_gl PRIVATE ${XRANDR_INCLUDE_DIR} ${X11_INCLUDE_DIR})
endif()
