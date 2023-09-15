# Copyright 2020-2022, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

add_executable(
	sdl-test_gl
	sdl_compositor.c
	sdl_device.c
	sdl_hack_stubs.c
	sdl_instance.c
	sdl_internal.h
	sdl_internal.hpp
	sdl_main.c
	sdl_program.cpp
	sdl_swapchain.c
	)
add_sanitizers(sdl-test_gl)

target_link_libraries(
	sdl-test_gl
	PRIVATE
		xrt-external-imgui-sdl2_gl
		aux_os_gl
		aux_ogl_gl
		aux_util_gl
		comp_util_gl
		comp_multi_gl
		ipc_server_gl
		st_gui_gl
		drv_includes_gl
		drv_simulated_gl
	)

if(WIN32)
	target_link_libraries(gui_gl PRIVATE SDL2::SDL2main)
endif()

install(TARGETS sdl-test_gl RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
