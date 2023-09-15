# Copyright 2020, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

add_library(monado-service_gl MODULE service_target.cpp)

target_link_libraries(
	monado-service_gl
	PRIVATE
		aux_util_gl
		st_prober_gl
		ipc_server_gl
		comp_main_gl
		target_lists_gl
		target_instance_gl
		xrt-external-jni-wrap_gl
	)
