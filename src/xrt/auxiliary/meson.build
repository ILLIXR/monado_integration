# Copyright 2019, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

aux_include = include_directories('.')

lib_aux_util = static_library(
	'aux_util',
	files(
		'util/u_misc.c',
		'util/u_misc.h',
		'util/u_debug.c',
		'util/u_debug.h',
		'util/u_device.c',
		'util/u_device.h',
		'util/u_distortion_mesh.c',
		'util/u_distortion_mesh.h',
		'util/u_documentation.h',
		'util/u_format.c',
		'util/u_format.h',
		'util/u_frame.c',
		'util/u_frame.h',
		'util/u_hashmap.cpp',
		'util/u_hashmap.h',
		'util/u_hashset.cpp',
		'util/u_hashset.h',
		'util/u_json.c',
		'util/u_json.h',
		'util/u_sink.h',
		'util/u_sink_converter.c',
		'util/u_sink_deinterleaver.c',
		'util/u_sink_queue.c',
		'util/u_sink_quirk.c',
		'util/u_sink_split.c',
		'util/u_time.cpp',
		'util/u_time.h',
		'util/u_var.cpp',
		'util/u_var.h',
	),
	include_directories: [
		xrt_include,
		external_include,
	],
)

aux_util = declare_dependency(
	include_directories: aux_include,
	link_with: lib_aux_util,
)

lib_aux_ogl = static_library(
	'aux_ogl',
	files(
		'ogl/ogl_api.h',
		'ogl/ogl_api.c',
		'ogl/ogl_documentation.h',
	),
	include_directories: [
		xrt_include,
		external_include,
	],
)

aux_ogl = declare_dependency(
	include_directories: aux_include,
	link_with: lib_aux_ogl,
)

lib_aux_os = static_library(
	'aux_os',
	files(
		'os/os_documentation.h',
		'os/os_hid.h',
		'os/os_hid_hidraw.c',
		'os/os_threading.h',
		'os/os_time.h',
	),
	include_directories: xrt_include,
)

aux_os = declare_dependency(
	include_directories: aux_include,
	link_with: lib_aux_os,
)

lib_aux_math = static_library(
	'aux_math',
	files(
		'math/m_api.h',
		'math/m_base.cpp',
		'math/m_eigen_interop.hpp',
		'math/m_filter_fifo.c',
		'math/m_filter_fifo.h',
		'math/m_hash.cpp',
		'math/m_optics.c',
		'math/m_quatexpmap.cpp',
		'math/m_vec2.h',
		'math/m_vec3.h',
	),
	include_directories: xrt_include,
	dependencies: [eigen3],
)

aux_math = declare_dependency(
	include_directories: aux_include,
	link_with: lib_aux_math,
)

tracking_srcs = [
	'tracking/t_imu.h',
	'tracking/t_imu_fusion.hpp',
	'tracking/t_imu.cpp',
	'tracking/t_lowpass.hpp',
	'tracking/t_lowpass_vector.hpp',
	'tracking/t_tracking.h',
]
tracking_deps = [eigen3]

if build_tracking
	tracking_srcs += [
		'tracking/t_calibration.cpp',
		'tracking/t_calibration_opencv.hpp',
		'tracking/t_convert.cpp',
		'tracking/t_debug_hsv_filter.cpp',
		'tracking/t_debug_hsv_picker.cpp',
		'tracking/t_debug_hsv_viewer.cpp',
		'tracking/t_file.cpp',
		'tracking/t_fusion.hpp',
		'tracking/t_helper_debug_sink.hpp',
		'tracking/t_hsv_filter.c',
		'tracking/t_kalman.cpp',
		'tracking/t_tracker_psmv.cpp',
		'tracking/t_tracker_psmv_fusion.hpp',
		'tracking/t_tracker_psvr.cpp',
	]
	tracking_deps += [opencv]
endif

lib_aux_tracking = static_library(
	'aux_tracking',
	files(tracking_srcs),
	include_directories: [xrt_include, external_include],
	dependencies: tracking_deps
)

aux_tracking = declare_dependency(
	include_directories: aux_include,
	link_with: lib_aux_tracking,
)

all_aux = [aux_util, aux_os, aux_math, aux_tracking]

aux = declare_dependency(dependencies: all_aux)
