# Copyright 2019-2022, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

###
# Main code
#
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)

add_library(
	st_oxr_gl STATIC
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
	oxr_dpad.c
	oxr_event.c
	oxr_extension_support.h
	oxr_handle_base.c
	oxr_input.c
	oxr_input_transform.c
	oxr_input_transform.h
	oxr_instance.c
	oxr_logger.c
	oxr_logger.h
	oxr_objects.h
	oxr_path.c
	oxr_pretty_print.c
	oxr_pretty_print.h
	oxr_session.c
	oxr_session_frame_end.c
	oxr_space.c
	oxr_swapchain.c
	oxr_system.c
	oxr_two_call.h
	oxr_verify.c
	oxr_xdev.c
	)

if(XRT_HAVE_VULKAN)
	target_compile_definitions(st_oxr_gl PRIVATE XR_USE_GRAPHICS_API_VULKAN)
	target_sources(st_oxr_gl PRIVATE oxr_session_gfx_vk.c oxr_swapchain_vk.c oxr_vulkan.c)
	target_link_libraries(st_oxr_gl PUBLIC Vulkan::Vulkan)
endif()

if(XRT_HAVE_OPENGL)
	target_compile_definitions(st_oxr_gl PRIVATE XR_USE_GRAPHICS_API_OPENGL)
endif()
if(XRT_HAVE_OPENGLES)
	target_compile_definitions(st_oxr_gl PRIVATE XR_USE_GRAPHICS_API_OPENGL_ES)
endif()

if(XRT_HAVE_OPENGL OR XRT_HAVE_OPENGLES)
	target_sources(st_oxr_gl PRIVATE oxr_swapchain_gl.c)
	target_link_libraries(st_oxr_gl PUBLIC aux_ogl_gl)
endif()

if(XRT_HAVE_OPENGL_GLX AND XRT_HAVE_XLIB)
	target_compile_definitions(st_oxr_gl PRIVATE XR_USE_PLATFORM_XLIB)
	target_sources(st_oxr_gl PRIVATE oxr_session_gfx_gl_xlib.c)
endif()

if(XRT_HAVE_EGL)
	target_compile_definitions(st_oxr_gl PRIVATE XR_USE_PLATFORM_EGL)
	target_sources(st_oxr_gl PRIVATE oxr_session_gfx_egl.c)
endif()

if(XRT_HAVE_D3D11 OR XRT_HAVE_D3D12)
	target_sources(st_oxr_gl PRIVATE oxr_d3d.cpp)
	target_link_libraries(st_oxr_gl PRIVATE aux_d3d_gl)
endif()

if(XRT_HAVE_D3D11)
	target_compile_definitions(st_oxr_gl PRIVATE XR_USE_GRAPHICS_API_D3D11)
	target_sources(st_oxr_gl PRIVATE oxr_session_gfx_d3d11.c oxr_swapchain_d3d11.c oxr_d3d11.cpp)
endif()

if(XRT_HAVE_D3D12)
	target_compile_definitions(st_oxr_gl PRIVATE XR_USE_GRAPHICS_API_D3D12)
	target_sources(st_oxr_gl PRIVATE oxr_session_gfx_d3d12.c oxr_swapchain_d3d12.c oxr_d3d12.cpp)
	target_link_libraries(st_oxr PRIVATE aux_d3d_gl)
endif()

if(ANDROID)
	target_compile_definitions(st_oxr_gl PRIVATE XR_USE_PLATFORM_ANDROID)
	target_sources(st_oxr_gl PRIVATE oxr_session_gfx_gles_android.c)
	target_link_libraries(st_oxr_gl PRIVATE aux_android_gl)
endif()
if(WIN32)
	target_link_libraries(st_oxr_gl PRIVATE kernel32)
	target_compile_definitions(st_oxr_gl PRIVATE XR_USE_PLATFORM_WIN32)
endif()

if(WIN32 AND XRT_HAVE_OPENGL)
	target_sources(st_oxr_gl PRIVATE oxr_session_gfx_gl_win32.c)
endif()

if(WIN32)
	target_compile_definitions(st_oxr_gl PRIVATE XR_USE_PLATFORM_WIN32)
endif()

target_link_libraries(
	st_oxr_gl
	PRIVATE
		xrt-interfaces_gl
		xrt-external-openxr_gl
		aux_util_gl
		aux_math_gl
		aux_generated_bindings_gl
		comp_client_gl
		aux-includes_gl
	PUBLIC aux_os_gl Threads::Threads
	)

if(XRT_FEATURE_RENDERDOC)
	target_link_libraries(st_oxr_gl PUBLIC xrt-external-renderdoc_gl)
	if(XRT_HAVE_LINUX)
		target_link_libraries(st_oxr_gl PRIVATE ${CMAKE_DL_LIBS})
	endif()
endif()

target_include_directories(
	st_oxr_gl
	PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..
	)
