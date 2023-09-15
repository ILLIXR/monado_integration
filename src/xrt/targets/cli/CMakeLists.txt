# Copyright 2019-2021, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

######
# Create a cli interface for Monado.

add_executable(
	cli_gl
	cli_cmd_lighthouse.c
	cli_cmd_probe.c
	cli_cmd_slambatch.c
	cli_cmd_test.c
	cli_common.h
	cli_main.c
	)
add_sanitizers(cli_gl)

if(NOT WIN32)
	# No getline on Windows, so until we have a portable impl
	target_sources(cli_gl PRIVATE cli_cmd_calibrate.c)
endif()

set_target_properties(cli_gl PROPERTIES OUTPUT_NAME monado-cli_gl PREFIX "")

target_link_libraries(
	cli_gl
	PRIVATE
		aux_os_gl
		aux_util_gl
		aux_math_gl
		target_instance_no_comp_gl
		drv_includes_gl
	)

install(TARGETS cli_gl RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
