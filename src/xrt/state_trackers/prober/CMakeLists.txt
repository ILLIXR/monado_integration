# Copyright 2019-2021, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

add_library(
	st_prober_gl STATIC
	p_documentation.h
	p_dump.c
	p_prober.c
	p_prober.h
	p_tracking.c
	)

target_link_libraries(st_prober_gl PUBLIC xrt-interfaces_gl)
target_link_libraries(
	st_prober_gl
	PRIVATE
		drv_includes_gl
		drv_multi_gl
		aux_util_gl
		aux_os_gl
		aux_tracking_gl
	)

# Add libudev
if(XRT_HAVE_LIBUDEV)
	target_sources(st_prober_gl PRIVATE p_udev.c)
	target_include_directories(st_prober_gl PRIVATE ${UDEV_INCLUDE_DIRS})
	target_link_libraries(st_prober_gl PRIVATE ${UDEV_LIBRARIES})
endif()

# Add libusb
if(XRT_HAVE_LIBUSB)
	target_sources(st_prober_gl PRIVATE p_libusb.c)
	target_include_directories(st_prober_gl PUBLIC ${LIBUSB1_INCLUDE_DIRS})
	target_link_libraries(st_prober_gl PRIVATE ${LIBUSB1_LIBRARIES})
endif()

# Add libuvc
if(XRT_HAVE_LIBUVC)
	target_sources(st_prober_gl PRIVATE p_libuvc.c)
	target_include_directories(st_prober_gl PRIVATE ${LIBUVC_INCLUDES})
	target_link_libraries(st_prober_gl PRIVATE ${LIBUVC_LIBRARIES})
endif()

if(XRT_HAVE_V4L2)
	# Uses v4l2_fs_create
	target_link_libraries(st_prober_gl PRIVATE drv_v4l2_gl)
endif()

if(XRT_BUILD_DRIVER_VF)
	target_link_libraries(st_prober_gl PRIVATE drv_vf_gl)
endif()
