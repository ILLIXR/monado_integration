# Copyright 2019-2021, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

######
# Create a small SDL2 based GUI for Monado.

add_executable(gui_gl gui_sdl2.c gui_sdl2_imgui.c gui_sdl2_main.c)

set_target_properties(gui_gl PROPERTIES OUTPUT_NAME monado-gui_gl PREFIX "")
add_sanitizers(gui_gl)

target_link_libraries(
	gui_gl
	PRIVATE
		aux_os_gl
		aux_ogl_gl
		aux_util_gl
		aux_math_gl
		st_gui_gl
		target_instance_no_comp_gl
		xrt-external-imgui-sdl2_gl
	)
target_include_directories(gui_gl PUBLIC ${SDL2_INCLUDE_DIRS})
if(WIN32)
	target_link_libraries(gui_gl PRIVATE SDL2::SDL2main)
endif()

install(TARGETS gui_gl RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
