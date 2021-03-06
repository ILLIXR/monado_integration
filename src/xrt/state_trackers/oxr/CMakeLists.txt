# Copyright 2019, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../../include
	${CMAKE_CURRENT_SOURCE_DIR}/../../auxiliary
	${CMAKE_CURRENT_SOURCE_DIR}/../../../external
	)

set(OXR_SOURCE_FILES
	oxr_api_action.c
	oxr_api_funcs.h
	oxr_api_instance.c
	oxr_api_negotiate.c
	oxr_api_session.c
	oxr_api_space.c
	oxr_api_swapchain.c
	oxr_api_system.c
	oxr_api_verify.h
	oxr_binding.c
	oxr_chain.h
	oxr_event.c
	oxr_extension_support.h
	oxr_handle_base.c
	oxr_input.c
	oxr_instance.c
	oxr_logger.c
	oxr_logger.h
	oxr_objects.h
	oxr_path.c
	oxr_session.c
	oxr_session_vk.c
	oxr_space.c
	oxr_swapchain.c
	oxr_swapchain_vk.c
	oxr_system.c
	oxr_two_call.h
	oxr_verify.c
	oxr_vulkan.c
	oxr_xdev.c
	)

if(BUILD_WITH_OPENGL)
	add_definitions(-DXR_USE_GRAPHICS_API_OPENGL -DXR_USE_GRAPHICS_API_OPENGL_ES)
	list(APPEND OXR_SOURCE_FILES
		oxr_session_gl.c
		oxr_swapchain_gl.c
		)
endif()

if(BUILD_WITH_XLIB)
	add_definitions(-DXR_USE_PLATFORM_XLIB)
endif()

if(BUILD_WITH_EGL)
	add_definitions(-DXR_USE_PLATFORM_EGL)
	list(APPEND OXR_SOURCE_FILES oxr_session_egl.c)
endif()

# Use OBJECT to not create a archive, since it just gets in the way.
add_library(st_oxr OBJECT ${OXR_SOURCE_FILES})

target_include_directories(st_oxr SYSTEM
	PRIVATE
	${VULKAN_INCLUDE_DIR}
	)
