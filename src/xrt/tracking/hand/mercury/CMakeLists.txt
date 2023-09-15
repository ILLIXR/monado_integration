# Copyright 2019-2022, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

# Mercury hand tracking library!

add_subdirectory(kine_lm)
add_subdirectory(kine_ccdik)

add_library(t_ht_mercury_model_gl STATIC hg_model.cpp)

target_link_libraries(t_ht_mercury_model_gl PRIVATE aux_math_gl aux_tracking_gl aux_os_gl aux_util_gl)

target_include_directories(
	t_ht_mercury_model_gl SYSTEM PRIVATE ${OpenCV_INCLUDE_DIRS} ${EIGEN3_INCLUDE_DIR}
	)

target_link_libraries(
	t_ht_mercury_model_gl
	PRIVATE
		aux_math_gl
		aux_tracking_gl
		aux_os_gl
		aux_util_gl
		${OpenCV_LIBRARIES}
		ONNXRuntime::ONNXRuntime
	)

add_library(t_ht_mercury_gl STATIC hg_sync.cpp hg_sync.hpp hg_interface.h kine_common.hpp)

target_link_libraries(
	t_ht_mercury_gl
	PUBLIC aux-includes_gl xrt-external-cjson_gl
	PRIVATE
		aux_math_gl
		aux_tracking_gl
		aux_os_gl
		aux_util_gl
		ONNXRuntime::ONNXRuntime
		t_ht_mercury_kine_lm_gl
		t_ht_mercury_kine_ccdik_gl
		t_ht_mercury_model_gl
		# ncnn # Soon...
		${OpenCV_LIBRARIES}
	)

if(XRT_HAVE_OPENCV)
	target_include_directories(
		t_ht_mercury_gl SYSTEM PRIVATE ${OpenCV_INCLUDE_DIRS} ${EIGEN3_INCLUDE_DIR}
		)
	target_link_libraries(t_ht_mercury_gl PUBLIC ${OpenCV_LIBRARIES})
endif()

# Below is entirely just so that tests can find us.
add_library(t_ht_mercury_includes_gl INTERFACE)

target_include_directories(
	t_ht_mercury_includes_gl INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
	)
