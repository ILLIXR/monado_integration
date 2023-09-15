# Copyright 2019-2022, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

add_library(aux_vive_gl STATIC vive_config.h vive_config.c vive_bindings.c vive_bindings.h)
target_link_libraries(
	aux_vive_gl
	PRIVATE
		xrt-interfaces_gl
		aux_util_gl
		aux_math_gl
		aux_tracking_gl
		xrt-external-cjson_gl
		${ZLIB_LIBRARIES}
	)
target_include_directories(aux_vive_gl PRIVATE ${ZLIB_INCLUDE_DIRS})
