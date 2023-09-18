# Copyright 2022, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

add_subdirectory(old_rgb)
add_subdirectory(mercury)

###
# Async wrapper around sync helper.
#
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)

add_library(hand_async_gl STATIC t_hand_tracking_async.c)
target_link_libraries(hand_async_gl PRIVATE xrt-interfaces_gl aux_os_gl aux_util_gl aux_math_gl PUBLIC Threads::Threads)
