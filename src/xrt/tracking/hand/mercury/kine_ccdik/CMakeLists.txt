# Copyright 2022, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

add_library(t_ht_mercury_kine_ccdik_gl STATIC ccdik_interface.hpp ccdik_main.cpp)

target_link_libraries(t_ht_mercury_kine_ccdik_gl PRIVATE aux_math_gl aux_tracking_gl aux_os_gl aux_util_gl)

target_include_directories(t_ht_mercury_kine_ccdik_gl SYSTEM PRIVATE ${EIGEN3_INCLUDE_DIR})
