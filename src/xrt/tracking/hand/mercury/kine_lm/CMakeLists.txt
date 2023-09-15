# Copyright 2022, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

add_library(t_ht_mercury_kine_lm_gl STATIC lm_interface.hpp lm_main.cpp)

target_link_libraries(
	t_ht_mercury_kine_lm_gl
	PRIVATE
		aux_math_gl
		aux_tracking_gl
		aux_os_gl
		aux_util_gl
		xrt-external-tinyceres_gl
	)

target_include_directories(t_ht_mercury_kine_lm_gl SYSTEM PRIVATE ${EIGEN3_INCLUDE_DIR})

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
	target_compile_options(t_ht_mercury_kine_lm_gl PRIVATE -ftemplate-backtrace-limit=20)
endif()

# Below is entirely just so that tests can find us.
add_library(t_ht_mercury_kine_lm_includes_gl INTERFACE)

target_include_directories(
	t_ht_mercury_kine_lm_includes_gl INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
						${CMAKE_CURRENT_BINARY_DIR}
	)
