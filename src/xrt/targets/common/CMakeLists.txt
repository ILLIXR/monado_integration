# Copyright 2019, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../../auxiliary
	${CMAKE_CURRENT_SOURCE_DIR}/../../include
	${CMAKE_CURRENT_SOURCE_DIR}/../../drivers
	)

set(SOURCE_FILES
	target_lists.c
	)

add_library(target_lists OBJECT ${SOURCE_FILES})
