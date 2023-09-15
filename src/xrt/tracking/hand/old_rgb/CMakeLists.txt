# Copyright 2019-2022, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

# Old RGB hand tracking library.
add_library(
	t_ht_old_rgb_gl STATIC
	rgb_hand_math.hpp
	rgb_image_math.hpp
	rgb_interface.h
	rgb_model.hpp
	rgb_nms.hpp
	rgb_sync.cpp
	rgb_sync.hpp
	)
target_link_libraries(
	t_ht_old_rgb_gl
	PUBLIC aux-includes_gl xrt-external-cjson_gl
	PRIVATE
		aux_math_gl
		aux_tracking_gl
		aux_os_gl
		aux_util_gl
		ONNXRuntime::ONNXRuntime
		${OpenCV_LIBRARIES}
	)
if(XRT_HAVE_OPENCV)
	target_include_directories(
		t_ht_old_rgb_gl SYSTEM PRIVATE ${OpenCV_INCLUDE_DIRS} ${EIGEN3_INCLUDE_DIR}
		)
	target_link_libraries(t_ht_old_rgb_gl PUBLIC ${OpenCV_LIBRARIES})
endif()
