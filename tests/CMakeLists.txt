# Copyright 2018-2022, Collabora, Ltd.
#
# SPDX-License-Identifier: BSL-1.0

# Catch2 main test driver
add_library(tests_main_gl STATIC tests_main.cpp)
target_link_libraries(tests_main_gl PUBLIC xrt-external-catch2_gl)
if(ANDROID)
	target_link_libraries(tests_main_gl PUBLIC log)
endif()

set(tests
    tests_cxx_wrappers
    tests_deque
    tests_generic_callbacks
    tests_history_buf
    tests_id_ringbuffer
    tests_input_transform
    tests_json
    tests_lowpass_float
    tests_lowpass_integer
    tests_pacing
    tests_quatexpmap
    tests_quat_change_of_basis
    tests_rational
    tests_vector
    tests_worker
    tests_pose
	)
if(XRT_HAVE_D3D11)
	list(APPEND tests tests_aux_d3d_d3d11 tests_comp_client_d3d11)
endif()
if(XRT_HAVE_D3D12)
	list(APPEND tests tests_comp_client_d3d12)
endif()
if(XRT_HAVE_VULKAN)
	list(APPEND tests tests_comp_client_vulkan)
endif()
if(XRT_HAVE_OPENGL
   AND XRT_HAVE_OPENGL_GLX
   AND XRT_HAVE_SDL2
   AND (WIN32 OR VK_USE_PLATFORM_XLIB_XRANDR_EXT)
	)
	set(_have_opengl_test ON)
	list(APPEND tests tests_comp_client_opengl)
endif()
if(XRT_BUILD_DRIVER_HANDTRACKING)
	list(APPEND tests tests_levenbergmarquardt)
endif()

foreach(testname ${tests})
	add_executable(${testname} ${testname}.cpp)
	target_link_libraries(${testname} PRIVATE tests_main_gl)
	target_link_libraries(${testname} PRIVATE aux_util_gl)
	add_test(NAME ${testname} COMMAND ${testname} --success)
endforeach()

# For tests that require more than just aux_util, link those other libs down here.

target_link_libraries(tests_cxx_wrappers PRIVATE xrt-interfaces_gl)
target_link_libraries(tests_history_buf PRIVATE aux_math_gl)
target_link_libraries(tests_input_transform PRIVATE st_oxr_gl xrt-interfaces_gl xrt-external-openxr_gl)
target_link_libraries(tests_lowpass_float PRIVATE aux_math_gl)
target_link_libraries(tests_lowpass_integer PRIVATE aux_math_gl)
target_link_libraries(tests_quatexpmap PRIVATE aux_math_gl)
target_link_libraries(tests_rational PRIVATE aux_math_gl)
target_link_libraries(tests_pose PRIVATE aux_math_gl)
target_link_libraries(tests_quat_change_of_basis PRIVATE aux_math_gl)

target_include_directories(tests_quat_change_of_basis SYSTEM PRIVATE ${EIGEN3_INCLUDE_DIR})

if(XRT_BUILD_DRIVER_HANDTRACKING)
	target_link_libraries(
		tests_levenbergmarquardt
		PRIVATE
			aux_math_gl
			t_ht_mercury_includes_gl
			t_ht_mercury_kine_lm_includes_gl
			t_ht_mercury_gl
			t_ht_mercury_kine_lm_gl
		)
endif()

if(XRT_HAVE_D3D11)
	target_link_libraries(tests_aux_d3d_d3d11 PRIVATE aux_d3d_gl)
	target_link_libraries(tests_comp_client_d3d11 PRIVATE comp_client_gl comp_mock_gl)
endif()

if(XRT_HAVE_D3D12)
	target_link_libraries(tests_comp_client_d3d12 PRIVATE comp_client_gl comp_mock_gl)
endif()

if(XRT_HAVE_VULKAN)
	target_link_libraries(
		tests_comp_client_vulkan PRIVATE comp_client_gl comp_mock_gl comp_util_gl aux_vk_gl
		)
endif()

if(_have_opengl_test)
	target_link_libraries(
		tests_comp_client_opengl PRIVATE comp_client_gl comp_mock_gl aux_ogl_gl ${SDL2_LIBRARIES}
		)
	target_include_directories(tests_comp_client_opengl PRIVATE ${SDL2_INCLUDE_DIRS})
endif()
