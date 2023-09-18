# Copyright 2019-2022, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads)

add_library(
	aux_os_gl STATIC
	os_ble.h
	os_documentation.h
	os_hid.h
	os_hid_hidraw.c
	os_threading.h
	)
target_link_libraries(aux_os_gl PUBLIC aux-includes_gl xrt-pthreads Threads::Threads)

if(XRT_HAVE_DBUS)
	target_sources(aux_os_gl PRIVATE os_ble_dbus.c)
	target_link_libraries(aux_os_gl PRIVATE ${DBUS_LIBRARIES})
	target_include_directories(aux_os_gl SYSTEM PRIVATE ${DBUS_INCLUDE_DIRS})
endif()
