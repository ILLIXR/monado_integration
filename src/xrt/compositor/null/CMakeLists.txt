# Copyright 2019-2022, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

add_library(comp_null_gl STATIC null_compositor.c null_compositor.h)
target_link_libraries(
	comp_null_gl
	PUBLIC xrt-interfaces_gl
	PRIVATE
		aux_util_gl
		aux_os_gl
		aux_vk_gl
		comp_util_gl
		comp_multi_gl
	)
target_include_directories(comp_null_gl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
