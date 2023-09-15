# Copyright 2019-2022, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

add_library(
	aux_gstreamer_gl STATIC
	gst_internal.h
	gst_sink.h
	gst_sink.c
	gst_pipeline.h
	gst_pipeline.c
	)
target_link_libraries(aux_gstreamer_gl PUBLIC aux-includes_gl)
target_link_libraries(aux_gstreamer_gl PRIVATE xrt-interfaces_gl aux_math_gl aux_os_gl ${GST_LIBRARIES})
target_include_directories(aux_gstreamer_gl PRIVATE ${GST_INCLUDE_DIRS})
