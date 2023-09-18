# Copyright 2019-2021, Collabora, Ltd.
#
# SPDX-License-Identifier: BSL-1.0
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)

set(ENABLED_HEADSET_DRIVERS)
set(ENABLED_DRIVERS)

add_library(drv_includes_gl INTERFACE)
target_include_directories(drv_includes_gl INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if(XRT_BUILD_DRIVER_ARDUINO)
	add_library(
		drv_arduino_gl STATIC arduino/arduino_device.c arduino/arduino_interface.h
				   arduino/arduino_prober.c
		)
	target_link_libraries(drv_arduino_gl PRIVATE xrt-interfaces_gl aux_util_gl aux_os_gl Threads::Threads)
	list(APPEND ENABLED_DRIVERS arduino)
endif()

add_library(drv_cemu_gl STATIC ht_ctrl_emu/ht_ctrl_emu.cpp ht_ctrl_emu/ht_ctrl_emu_interface.h)
target_link_libraries(drv_cemu_gl PRIVATE xrt-interfaces_gl aux_generated_bindings_gl aux_util_gl)
list(APPEND ENABLED_HEADSET_DRIVERS drv_cemu_gl)

if(XRT_BUILD_DRIVER_DAYDREAM)
	add_library(
		drv_daydream_gl STATIC daydream/daydream_device.c daydream/daydream_device.h
				    daydream/daydream_interface.h daydream/daydream_prober.c
		)
	target_link_libraries(drv_daydream_gl PRIVATE xrt-interfaces_gl aux_util_gl aux_os_gl Threads::Threads)
	list(APPEND ENABLED_DRIVERS daydream)
endif()

if(XRT_BUILD_DRIVER_DEPTHAI)

	add_library(drv_depthai_gl STATIC depthai/depthai_driver.cpp depthai/depthai_interface.h)
	target_link_libraries(
		drv_depthai_gl
		PRIVATE
			xrt-interfaces_gl
			aux_os_gl
			aux_tracking_gl
			${OpenCV_LIBRARIES}
			depthai::core
			XLink
			Threads::Threads
		)
	target_include_directories(drv_depthai_gl PRIVATE ${OpenCV_INCLUDE_DIRS})
	list(APPEND ENABLED_DRIVERS depthai)
endif()

if(XRT_BUILD_DRIVER_SIMULATED)
	add_library(
		drv_simulated_gl STATIC simulated/simulated_hmd.c simulated/simulated_interface.h
				     simulated/simulated_prober.c
		)
	target_link_libraries(drv_simulated_gl PRIVATE xrt-interfaces_gl aux_util_gl)
	list(APPEND ENABLED_HEADSET_DRIVERS simulated)
endif()

if(XRT_BUILD_DRIVER_TWRAP)
	add_library(drv_twrap_gl STATIC twrap/twrap_slam.c twrap/twrap_interface.h)
	target_link_libraries(drv_twrap_gl PRIVATE xrt-interfaces_gl aux_util_gl)
endif()

if(XRT_BUILD_DRIVER_QWERTY)
	add_library(
		drv_qwerty_gl STATIC
		qwerty/qwerty_device.c
		qwerty/qwerty_device.h
		qwerty/qwerty_interface.h
		qwerty/qwerty_prober.c
		qwerty/qwerty_sdl.c
		)
	target_link_libraries(drv_qwerty_gl PRIVATE xrt-interfaces_gl aux_util_gl ${SDL2_LIBRARIES})
	target_include_directories(drv_qwerty_gl PRIVATE ${SDL2_INCLUDE_DIRS})
	list(APPEND ENABLED_DRIVERS qwerty)

	add_library(drv_qwerty_includes_gl INTERFACE)
	target_include_directories(drv_qwerty_includes_gl INTERFACE qwerty)
endif()

if(XRT_BUILD_DRIVER_HDK)
	set(HDK_SOURCE_FILES)

	add_library(
		drv_hdk_gl STATIC hdk/hdk_device.cpp hdk/hdk_device.h hdk/hdk_interface.h
			       hdk/hdk_prober.c
		)
	target_link_libraries(drv_hdk_gl PRIVATE xrt-interfaces_gl aux_math_gl Threads::Threads)
	list(APPEND ENABLED_HEADSET_DRIVERS hdk)
endif()

if(XRT_BUILD_DRIVER_HYDRA)
	set(HYDRA_SOURCE_FILES)

	add_library(drv_hydra_gl STATIC hydra/hydra_driver.c hydra/hydra_interface.h)
	target_link_libraries(drv_hydra_gl PRIVATE xrt-interfaces_gl aux_os_gl aux_util_gl)
	list(APPEND ENABLED_DRIVERS hydra)
endif()

if(XRT_BUILD_DRIVER_NS)
	add_library(
		drv_ns_gl STATIC
		north_star/distortion_3d/utility_northstar.h
		north_star/distortion_3d/deformation_northstar.h
		north_star/distortion_3d/deformation_northstar.cpp
		north_star/ns_hmd.h
		north_star/ns_hmd.c
		north_star/ns_interface.h
		)
	target_link_libraries(drv_ns_gl PRIVATE xrt-interfaces_gl aux_math_gl xrt-external-cjson_gl Threads::Threads)
	list(APPEND ENABLED_HEADSET_DRIVERS ns)
endif()

if(XRT_BUILD_DRIVER_ULV2)
	add_library(drv_ulv2_gl STATIC ultraleap_v2/ulv2_driver.cpp ultraleap_v2/ulv2_interface.h)
	target_link_libraries(drv_ulv2_gl PRIVATE xrt-interfaces_gl aux_util_gl aux_math_gl LeapV2::LeapV2 Threads::Threads)
endif()

if(XRT_BUILD_DRIVER_OHMD)
	add_library(
		drv_ohmd_gl STATIC ohmd/oh_device.c ohmd/oh_device.h ohmd/oh_interface.h
				ohmd/oh_prober.c
		)
	target_link_libraries(drv_ohmd_gl PRIVATE xrt-interfaces_gl OpenHMD::OpenHMD aux_util_gl aux_math_gl)
	list(APPEND ENABLED_HEADSET_DRIVERS openhmd)
endif()

if(XRT_BUILD_DRIVER_OPENGLOVES)
	add_library(
		drv_opengloves_gl STATIC
		opengloves/opengloves_interface.h
		opengloves/opengloves_device.c
		opengloves/opengloves_prober.c
		opengloves/opengloves_device.h
		opengloves/communication/serial/opengloves_serial.h
		opengloves/communication/serial/opengloves_serial.c
		opengloves/encoding/alpha_encoding.h
		opengloves/encoding/alpha_encoding.cpp
		opengloves/encoding/encoding.h
		opengloves/communication/bluetooth/opengloves_bt_serial.h
		opengloves/communication/bluetooth/opengloves_bt_serial.c
		opengloves/communication/opengloves_communication.h
		opengloves/communication/serial/opengloves_prober_serial.h
		opengloves/communication/serial/opengloves_prober_serial.c
		opengloves/communication/bluetooth/opengloves_prober_bt.h
		opengloves/communication/bluetooth/opengloves_prober_bt.c
		)
	target_link_libraries(drv_opengloves_gl PRIVATE xrt-interfaces_gl aux_util_gl aux_os_gl bluetooth Threads::Threads)
	list(APPEND ENABLED_DRIVERS opengloves)

endif()

if(XRT_BUILD_DRIVER_PSMV)
	add_library(drv_psmv_gl STATIC psmv/psmv_driver.c psmv/psmv_interface.h)
	target_link_libraries(
		drv_psmv_gl
		PRIVATE xrt-interfaces_gl
		PUBLIC aux_os_gl aux_tracking_gl Threads::Threads
		)
	list(APPEND ENABLED_DRIVERS psmv)
endif()

if(XRT_BUILD_DRIVER_PSVR)
	add_library(
		drv_psvr_gl STATIC
		psvr/psvr_device.c
		psvr/psvr_device.h
		psvr/psvr_interface.h
		psvr/psvr_packet.c
		psvr/psvr_prober.c
		)
	target_link_libraries(drv_psvr_gl PRIVATE xrt-interfaces_gl HIDAPI::hidapi aux_util_gl Threads::Threads)
	target_include_directories(drv_psvr_gl PRIVATE ${HIDAPI_INCLUDE_DIRS})
	list(APPEND ENABLED_HEADSET_DRIVERS psvr)
endif()

if(XRT_BUILD_DRIVER_REALSENSE)
	add_library(
		drv_realsense_gl STATIC
		realsense/rs_ddev.c
		realsense/rs_hdev.c
		realsense/rs_prober.c
		realsense/rs_driver.h
		realsense/rs_interface.h
		)
	target_link_libraries(drv_realsense_gl PRIVATE xrt-interfaces_gl realsense2::realsense2 aux_util_gl Threads::Threads)
	list(APPEND ENABLED_HEADSET_DRIVERS rs)
endif()

if(XRT_BUILD_DRIVER_REMOTE)
	add_library(
		drv_remote_gl STATIC
		remote/r_device.c
		remote/r_hmd.c
		remote/r_hub.c
		remote/r_interface.h
		remote/r_internal.h
		)
	target_link_libraries(drv_remote_gl PRIVATE xrt-interfaces_gl aux_util_gl aux_vive_gl Threads::Threads)
	list(APPEND ENABLED_HEADSET_DRIVERS remote)
endif()

if(XRT_BUILD_DRIVER_RIFT_S)
	add_library(
		drv_rift_s_gl STATIC
		rift_s/rift_s_interface.h
		rift_s/rift_s_camera.c
		rift_s/rift_s_camera.h
		rift_s/rift_s_controller.c
		rift_s/rift_s_controller.h
		rift_s/rift_s_firmware.c
		rift_s/rift_s_firmware.h
		rift_s/rift_s_hmd.c
		rift_s/rift_s_hmd.h
		rift_s/rift_s_protocol.c
		rift_s/rift_s_protocol.h
		rift_s/rift_s_radio.c
		rift_s/rift_s_radio.h
		rift_s/rift_s_tracker.c
		rift_s/rift_s_tracker.h
		rift_s/rift_s_util.cpp
		rift_s/rift_s_util.h
		rift_s/rift_s.c
		rift_s/rift_s.h
		)
	target_include_directories(drv_rift_s_gl SYSTEM PRIVATE ${EIGEN3_INCLUDE_DIR})
	target_link_libraries(
		drv_rift_s_gl
		PRIVATE
			xrt-interfaces_gl
			aux_util_gl
			aux_math_gl
			xrt-external-cjson_gl
			xrt-external-tinyceres_gl
			Threads::Threads
		)
	list(APPEND ENABLED_HEADSET_DRIVERS rift-s)
endif()

set(VIVE_CONFIG_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/vive")
if(XRT_BUILD_DRIVER_VIVE)
	add_library(
		drv_vive_gl STATIC
		vive/vive_device.h
		vive/vive_device.c
		vive/vive_prober.h
		vive/vive_prober.c
		vive/vive_protocol.c
		vive/vive_protocol.h
		vive/vive_controller.h
		vive/vive_controller.c
		vive/vive_lighthouse.h
		vive/vive_lighthouse.c
		vive/vive_source.h
		vive/vive_source.c
		)
	target_link_libraries(
		drv_vive_gl
		PRIVATE
			xrt-interfaces_gl
			aux_os_gl
			aux_util_gl
			aux_math_gl
			xrt-external-cjson_gl
			aux_vive_gl
			Threads::Threads
		)
	target_link_libraries(drv_vive_gl PRIVATE ${ZLIB_LIBRARIES})
	target_include_directories(drv_vive_gl PRIVATE ${ZLIB_INCLUDE_DIRS})
	list(APPEND ENABLED_HEADSET_DRIVERS vive)
endif()

if(XRT_HAVE_V4L2)
	add_library(drv_v4l2_gl STATIC v4l2/v4l2_driver.h v4l2/v4l2_driver.c)
	target_link_libraries(drv_v4l2_gl PRIVATE xrt-interfaces_gl aux_os_gl aux_util_gl)
	list(APPEND ENABLED_DRIVERS v4l2)
endif()

if(XRT_BUILD_DRIVER_VF)
	add_library(drv_vf_gl STATIC vf/vf_driver.c)
	target_link_libraries(drv_vf PRIVATE xrt-interfaces_gl aux_os_gl aux_util_gl ${GST_LIBRARIES} Threads::Threads)
	target_include_directories(drv_vf_gl PRIVATE ${GST_INCLUDE_DIRS})
	list(APPEND ENABLED_DRIVERS vf)
endif()

if(XRT_BUILD_DRIVER_HANDTRACKING)
	add_library(drv_ht_gl STATIC ht/ht_driver.c ht/ht_interface.h)
	target_link_libraries(
		drv_ht_gl
		PRIVATE
			xrt-interfaces_gl
			aux_os_gl
			aux_util_gl
			aux_math_gl
			t_ht_mercury_gl
			t_ht_old_rgb_gl
			hand_async_gl
		)

	target_include_directories(drv_ht_gl PRIVATE ${OpenCV_INCLUDE_DIRS} ${EIGEN3_INCLUDE_DIR})
	list(APPEND ENABLED_DRIVERS ht)
endif()

if(XRT_BUILD_DRIVER_SURVIVE)
	add_library(drv_survive_gl STATIC survive/survive_driver.c survive/survive_interface.h)
	target_link_libraries(
		drv_survive_gl
		PRIVATE
			xrt-interfaces_gl
			aux_os_gl
			aux_util_gl
			aux_math_gl
			aux_vive_gl
			PkgConfig::SURVIVE
			Threads::Threads
		)
	list(APPEND ENABLED_HEADSET_DRIVERS survive)
endif()

if(XRT_BUILD_DRIVER_ANDROID)
	add_library(
		drv_android_gl STATIC android/android_prober.c android/android_prober.h
				   android/android_sensors.c android/android_sensors.h
		)
	target_link_libraries(
		drv_android_gl
		PRIVATE
			xrt-interfaces_gl
			aux_util_gl
			aux_os_gl
			aux_android_gl
			${ANDROID_LIBRARY}
			Threads::Threads
		)
	list(APPEND ENABLED_DRIVERS android)
endif()

if(XRT_BUILD_DRIVER_ILLIXR)
	find_package(spdlog REQUIRED)
	add_library(
		drv_illixr_gl STATIC
		illixr/illixr_device.cpp
		illixr/illixr_interface.h
		illixr/illixr_prober.c
		illixr/illixr_component.cpp
		illixr/illixr_component.h
		)
	target_link_libraries(drv_illixr_gl PUBLIC ${CMAKE_DL_LIBS} xrt-interfaces_gl aux_util_gl aux_os_gl spdlog::spdlog Threads::Threads)
	target_include_directories(drv_illixr_gl PUBLIC ${ILLIXR_PATH})
	list(APPEND ENABLED_HEADSET_DRIVERS illixr)
endif()

add_library(drv_multi_gl STATIC multi_wrapper/multi.c multi_wrapper/multi.h)
target_link_libraries(drv_multi_gl PUBLIC xrt-interfaces_gl aux_util_gl)
list(APPEND ENABLED_HEADSET_DRIVERS drv_multi)

if(XRT_BUILD_DRIVER_WMR)
	add_library(
		drv_wmr_gl STATIC
		wmr/wmr_camera.h
		wmr/wmr_common.h
		wmr/wmr_config.c
		wmr/wmr_config.h
		wmr/wmr_bt_controller.c
		wmr/wmr_bt_controller.h
		wmr/wmr_hmd.c
		wmr/wmr_hmd.h
		wmr/wmr_interface.h
		wmr/wmr_prober.c
		wmr/wmr_protocol.c
		wmr/wmr_protocol.h
		wmr/wmr_controller_protocol.c
		wmr/wmr_controller_protocol.h
		wmr/wmr_source.c
		wmr/wmr_source.h
		)
	target_link_libraries(
		drv_wmr_gl
		PRIVATE
			xrt-interfaces_gl
			aux_util_gl
			aux_math_gl
			aux_tracking_gl
			xrt-external-cjson_gl
			Threads::Threads
		)
	list(APPEND ENABLED_HEADSET_DRIVERS wmr)

	# Can only build camera support with libusb
	if(XRT_HAVE_LIBUSB)
		target_sources(drv_wmr_gl PRIVATE wmr/wmr_camera.c)
		target_include_directories(drv_wmr_gl PUBLIC ${LIBUSB1_INCLUDE_DIRS})
		target_link_libraries(drv_wmr_gl PRIVATE ${LIBUSB1_LIBRARIES})
	endif()

	if(XRT_BUILD_DRIVER_HANDTRACKING)
		target_link_libraries(drv_wmr_gl PRIVATE drv_ht_gl)
		target_link_libraries(drv_wmr_gl PRIVATE drv_cemu_gl)
	endif()
endif()

if(XRT_BUILD_DRIVER_EUROC)
	add_library(
		drv_euroc_gl STATIC
		euroc/euroc_player.cpp
		euroc/euroc_driver.h
		euroc/euroc_device.c
		euroc/euroc_interface.h
		euroc/euroc_runner.c
		)
	target_link_libraries(
		drv_euroc_gl PRIVATE xrt-interfaces_gl aux_util_gl aux_tracking_gl ${OpenCV_LIBRARIES}
		)
	target_include_directories(drv_euroc_gl PRIVATE ${OpenCV_INCLUDE_DIRS} Threads::Threads)
	list(APPEND ENABLED_DRIVERS euroc)
endif()

if(XRT_BUILD_DRIVER_SIMULAVR)
	add_library(drv_svr_gl STATIC simula/svr_hmd.c simula/svr_interface.h)
	target_link_libraries(drv_svr_gl PRIVATE xrt-interfaces_gl aux_math_gl xrt-external-cjson_gl Threads::Threads)
	list(APPEND ENABLED_HEADSET_DRIVERS svr)
endif()

if(XRT_BUILD_SAMPLES)
	# We build the sample driver to make sure it stays valid,
	# but it never gets linked into a final target.
	add_library(
		drv_sample_gl STATIC sample/sample_hmd.c sample/sample_interface.h
				  sample/sample_prober.c
		)
	target_link_libraries(drv_sample_gl PRIVATE xrt-interfaces_gl aux_util_gl)
endif()

if(ENABLED_HEADSET_DRIVERS)
	set(ENABLED_DRIVERS ${ENABLED_HEADSET_DRIVERS} ${ENABLED_DRIVERS})
	list(SORT ENABLED_DRIVERS)
	string(REPLACE ";" " " ENABLED_DRIVERS "${ENABLED_DRIVERS}")
	message(STATUS "Enabled drivers: ${ENABLED_DRIVERS}")
else()
	message(FATAL_ERROR "You must enable at least one headset driver to build Monado.")
endif()
