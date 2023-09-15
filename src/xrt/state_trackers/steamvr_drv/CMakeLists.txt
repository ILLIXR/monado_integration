# Copyright 2020-2021, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

add_library(st_ovrd_gl STATIC ovrd_driver.cpp ovrd_interface.h)

target_include_directories(st_ovrd_gl INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
	st_ovrd_gl PRIVATE xrt-interfaces_gl xrt-external-openvr_gl aux_math_gl aux_generated_bindings_gl
	)
