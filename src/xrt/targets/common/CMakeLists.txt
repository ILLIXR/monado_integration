# Copyright 2019-2021, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

####
# Lists
#

add_library(
	target_lists_gl STATIC target_builder_interface.h target_builder_legacy.c target_lists.c
			    target_lists.h
	)
target_link_libraries(
	target_lists_gl
	PRIVATE
		xrt-interfaces_gl
		aux_util_gl # TODO Remove this after removing #include "util/u_time.h" from xrt_defines.h
		drv_includes_gl
	)
target_include_directories(target_lists_gl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

###
# Builders
#
if(XRT_BUILD_DRIVER_REMOTE)
	target_sources(target_lists_gl PRIVATE target_builder_remote.c)
endif()

if(XRT_BUILD_DRIVER_PSVR OR XRT_BUILD_DRIVER_PSMV)
	target_sources(target_lists_gl PRIVATE target_builder_rgb_tracking.c)
endif()

if(XRT_BUILD_DRIVER_RIFT_S)
	target_sources(target_lists_gl PRIVATE target_builder_rift_s.c)
endif()

if(XRT_BUILD_DRIVER_SURVIVE OR XRT_BUILD_DRIVER_VIVE)
	target_sources(target_lists_gl PRIVATE target_builder_lighthouse.c)
endif()

if(XRT_BUILD_DRIVER_SIMULAVR)
	target_sources(target_lists_gl PRIVATE target_builder_simulavr.c)
endif()

if(XRT_BUILD_DRIVER_NS)
	target_sources(target_lists_gl PRIVATE target_builder_north_star.c)
	target_link_libraries(target_lists_gl PRIVATE drv_ns_gl)
endif()

###
# Drivers
#

if(XRT_BUILD_DRIVER_ARDUINO)
	target_link_libraries(target_lists_gl PRIVATE drv_arduino_gl)
endif()

target_link_libraries(target_lists_gl PRIVATE drv_cemu_gl)

if(XRT_BUILD_DRIVER_DAYDREAM)
	target_link_libraries(target_lists_gl PRIVATE drv_daydream_gl)
endif()

if(XRT_BUILD_DRIVER_DEPTHAI)
	target_link_libraries(target_lists_gl PRIVATE drv_depthai_gl)
endif()

if(XRT_BUILD_DRIVER_SIMULATED)
	target_link_libraries(target_lists_gl PRIVATE drv_simulated_gl)
endif()

if(XRT_BUILD_DRIVER_HDK)
	target_link_libraries(target_lists_gl PRIVATE drv_hdk_gl)
endif()

if(XRT_BUILD_DRIVER_HYDRA)
	target_link_libraries(target_lists_gl PRIVATE drv_hydra_gl)
endif()

if(XRT_BUILD_DRIVER_NS)
	target_link_libraries(target_lists_gl PRIVATE drv_ns_gl)
endif()

if(XRT_BUILD_DRIVER_TWRAP)
	target_link_libraries(target_lists_gl PRIVATE drv_twrap_gl)
endif()

if(XRT_BUILD_DRIVER_ULV2)
	target_link_libraries(target_lists_gl PRIVATE drv_ulv2_gl)
endif()

if(XRT_BUILD_DRIVER_OHMD)
	target_link_libraries(target_lists_gl PRIVATE drv_ohmd_gl)
endif()

if(XRT_BUILD_DRIVER_OPENGLOVES)
	target_link_libraries(target_lists_gl PRIVATE drv_opengloves_gl)
endif()

if(XRT_BUILD_DRIVER_HANDTRACKING)
	target_link_libraries(target_lists_gl PRIVATE drv_ht_gl)
endif()

if(XRT_BUILD_DRIVER_PSMV)
	target_link_libraries(target_lists_gl PRIVATE drv_psmv_gl)
endif()

if(XRT_BUILD_DRIVER_PSVR)
	target_link_libraries(target_lists_gl PRIVATE drv_psvr_gl)
endif()

if(XRT_BUILD_DRIVER_REALSENSE)
	target_link_libraries(target_lists_gl PRIVATE drv_realsense_gl)
endif()

if(XRT_BUILD_DRIVER_REMOTE)
	target_link_libraries(target_lists_gl PRIVATE drv_remote_gl)
endif()

if(XRT_BUILD_DRIVER_RIFT_S)
	target_link_libraries(target_lists_gl PRIVATE drv_rift_s_gl)
endif()

if(XRT_HAVE_V4L2)
	target_link_libraries(target_lists_gl PRIVATE drv_v4l2_gl)
endif()

if(XRT_BUILD_DRIVER_VF)
	target_link_libraries(target_lists_gl PRIVATE drv_vf_gl)
endif()

if(XRT_BUILD_DRIVER_VIVE)
	target_link_libraries(target_lists_gl PRIVATE drv_vive_gl)
endif()

if(XRT_BUILD_DRIVER_SURVIVE)
	target_link_libraries(target_lists_gl PRIVATE drv_survive_gl)
endif()

if(XRT_BUILD_DRIVER_ANDROID)
	target_link_libraries(target_lists_gl PRIVATE drv_android_gl)
endif()

if(XRT_BUILD_DRIVER_ILLIXR)
	target_link_libraries(target_lists_gl PRIVATE drv_illixr_gl)
endif()

target_link_libraries(target_lists_gl PRIVATE drv_multi_gl)

if(XRT_BUILD_DRIVER_QWERTY)
	target_link_libraries(target_lists_gl PRIVATE drv_qwerty_gl)
endif()

if(XRT_BUILD_DRIVER_WMR)
	target_link_libraries(target_lists_gl PRIVATE drv_wmr_gl)
endif()

if(XRT_BUILD_DRIVER_EUROC)
	target_link_libraries(target_lists_gl PRIVATE drv_euroc_gl)
endif()

if(XRT_BUILD_DRIVER_SIMULAVR)
	target_link_libraries(target_lists_gl PRIVATE drv_svr_gl)
endif()

####
# Instance
#

if(XRT_FEATURE_COMPOSITOR_MAIN OR XRT_FEATURE_COMPOSITOR_NULL)
	add_library(target_instance_gl STATIC target_instance.c)
	target_link_libraries(
		target_instance_gl
		PRIVATE
			xrt-interfaces_gl
			aux_util_gl
			st_prober_gl
			target_lists_gl
			drv_includes_gl
		)
	target_include_directories(target_instance_gl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

	if(XRT_FEATURE_COMPOSITOR_MAIN)
		target_link_libraries(target_instance_gl PRIVATE comp_main_gl)
	endif()

	if(XRT_FEATURE_COMPOSITOR_NULL)
		target_link_libraries(target_instance_gl PRIVATE comp_null_gl)
	endif()
endif()

####
# Instance without Compositor
#

add_library(target_instance_no_comp_gl STATIC target_instance_no_comp.c)
target_link_libraries(
	target_instance_no_comp_gl
	PRIVATE
		xrt-interfaces_gl
		aux_util_gl
		st_prober_gl
		target_lists_gl
		drv_includes_gl
	)
target_include_directories(target_instance_no_comp_gl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
