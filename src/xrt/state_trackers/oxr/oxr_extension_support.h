// Copyright 2019-2022, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Macros for generating extension-related tables and code and
 * inspecting Monado's extension support.
 *
 * MOSTLY GENERATED CODE - see below!
 *
 * To add support for a new extension, edit and run generate_oxr_ext_support.py,
 * then run clang-format on this file. Two entire chunks of this file get
 * replaced by that script: comments indicate where they are.
 *
 * @author Ryan Pavlik <ryan.pavlik@collabora.com>
 */

#pragma once

#include "xrt/xrt_config_build.h"
#include "xrt/xrt_openxr_includes.h"

// beginning of GENERATED defines - do not modify - used by scripts

/*
 * XR_KHR_android_create_instance
 */
#if defined(XR_KHR_android_create_instance) && defined(XR_USE_PLATFORM_ANDROID)
#define OXR_HAVE_KHR_android_create_instance
#define OXR_EXTENSION_SUPPORT_KHR_android_create_instance(_) _(KHR_android_create_instance, KHR_ANDROID_CREATE_INSTANCE)
#else
#define OXR_EXTENSION_SUPPORT_KHR_android_create_instance(_)
#endif


/*
 * XR_KHR_binding_modification
 */
#if defined(XR_KHR_binding_modification)
#define OXR_HAVE_KHR_binding_modification
#define OXR_EXTENSION_SUPPORT_KHR_binding_modification(_) _(KHR_binding_modification, KHR_BINDING_MODIFICATION)
#else
#define OXR_EXTENSION_SUPPORT_KHR_binding_modification(_)
#endif


/*
 * XR_KHR_composition_layer_cube
 */
#if defined(XR_KHR_composition_layer_cube) && defined(XRT_FEATURE_OPENXR_LAYER_CUBE)
#define OXR_HAVE_KHR_composition_layer_cube
#define OXR_EXTENSION_SUPPORT_KHR_composition_layer_cube(_) _(KHR_composition_layer_cube, KHR_COMPOSITION_LAYER_CUBE)
#else
#define OXR_EXTENSION_SUPPORT_KHR_composition_layer_cube(_)
#endif


/*
 * XR_KHR_composition_layer_cylinder
 */
#if defined(XR_KHR_composition_layer_cylinder) && defined(XRT_FEATURE_OPENXR_LAYER_CYLINDER)
#define OXR_HAVE_KHR_composition_layer_cylinder
#define OXR_EXTENSION_SUPPORT_KHR_composition_layer_cylinder(_)                                                        \
	_(KHR_composition_layer_cylinder, KHR_COMPOSITION_LAYER_CYLINDER)
#else
#define OXR_EXTENSION_SUPPORT_KHR_composition_layer_cylinder(_)
#endif


/*
 * XR_KHR_composition_layer_depth
 */
#if defined(XR_KHR_composition_layer_depth) && defined(XRT_FEATURE_OPENXR_LAYER_DEPTH)
#define OXR_HAVE_KHR_composition_layer_depth
#define OXR_EXTENSION_SUPPORT_KHR_composition_layer_depth(_) _(KHR_composition_layer_depth, KHR_COMPOSITION_LAYER_DEPTH)
#else
#define OXR_EXTENSION_SUPPORT_KHR_composition_layer_depth(_)
#endif


/*
 * XR_KHR_composition_layer_equirect
 */
#if defined(XR_KHR_composition_layer_equirect) && defined(XRT_FEATURE_OPENXR_LAYER_EQUIRECT1)
#define OXR_HAVE_KHR_composition_layer_equirect
#define OXR_EXTENSION_SUPPORT_KHR_composition_layer_equirect(_)                                                        \
	_(KHR_composition_layer_equirect, KHR_COMPOSITION_LAYER_EQUIRECT)
#else
#define OXR_EXTENSION_SUPPORT_KHR_composition_layer_equirect(_)
#endif


/*
 * XR_KHR_composition_layer_equirect2
 */
#if defined(XR_KHR_composition_layer_equirect2) && defined(XRT_FEATURE_OPENXR_LAYER_EQUIRECT2)
#define OXR_HAVE_KHR_composition_layer_equirect2
#define OXR_EXTENSION_SUPPORT_KHR_composition_layer_equirect2(_)                                                       \
	_(KHR_composition_layer_equirect2, KHR_COMPOSITION_LAYER_EQUIRECT2)
#else
#define OXR_EXTENSION_SUPPORT_KHR_composition_layer_equirect2(_)
#endif


/*
 * XR_KHR_convert_timespec_time
 */
#if defined(XR_KHR_convert_timespec_time) && defined(XR_USE_TIMESPEC)
#define OXR_HAVE_KHR_convert_timespec_time
#define OXR_EXTENSION_SUPPORT_KHR_convert_timespec_time(_) _(KHR_convert_timespec_time, KHR_CONVERT_TIMESPEC_TIME)
#else
#define OXR_EXTENSION_SUPPORT_KHR_convert_timespec_time(_)
#endif


/*
 * XR_KHR_D3D11_enable
 */
#if defined(XR_KHR_D3D11_enable) && defined(XR_USE_GRAPHICS_API_D3D11)
#define OXR_HAVE_KHR_D3D11_enable
#define OXR_EXTENSION_SUPPORT_KHR_D3D11_enable(_) _(KHR_D3D11_enable, KHR_D3D11_ENABLE)
#else
#define OXR_EXTENSION_SUPPORT_KHR_D3D11_enable(_)
#endif


/*
 * XR_KHR_D3D12_enable
 */
#if defined(XR_KHR_D3D12_enable) && defined(XR_USE_GRAPHICS_API_D3D12)
#define OXR_HAVE_KHR_D3D12_enable
#define OXR_EXTENSION_SUPPORT_KHR_D3D12_enable(_) _(KHR_D3D12_enable, KHR_D3D12_ENABLE)
#else
#define OXR_EXTENSION_SUPPORT_KHR_D3D12_enable(_)
#endif


/*
 * XR_KHR_loader_init
 */
#if defined(XR_KHR_loader_init) && defined(XR_USE_PLATFORM_ANDROID)
#define OXR_HAVE_KHR_loader_init
#define OXR_EXTENSION_SUPPORT_KHR_loader_init(_) _(KHR_loader_init, KHR_LOADER_INIT)
#else
#define OXR_EXTENSION_SUPPORT_KHR_loader_init(_)
#endif


/*
 * XR_KHR_loader_init_android
 */
#if defined(XR_KHR_loader_init_android) && defined(OXR_HAVE_KHR_loader_init) && defined(XR_USE_PLATFORM_ANDROID)
#define OXR_HAVE_KHR_loader_init_android
#define OXR_EXTENSION_SUPPORT_KHR_loader_init_android(_) _(KHR_loader_init_android, KHR_LOADER_INIT_ANDROID)
#else
#define OXR_EXTENSION_SUPPORT_KHR_loader_init_android(_)
#endif


/*
 * XR_KHR_opengl_enable
 */
#if defined(XR_KHR_opengl_enable) && defined(XR_USE_GRAPHICS_API_OPENGL)
#define OXR_HAVE_KHR_opengl_enable
#define OXR_EXTENSION_SUPPORT_KHR_opengl_enable(_) _(KHR_opengl_enable, KHR_OPENGL_ENABLE)
#else
#define OXR_EXTENSION_SUPPORT_KHR_opengl_enable(_)
#endif


/*
 * XR_KHR_opengl_es_enable
 */
#if defined(XR_KHR_opengl_es_enable) && defined(XR_USE_GRAPHICS_API_OPENGL_ES)
#define OXR_HAVE_KHR_opengl_es_enable
#define OXR_EXTENSION_SUPPORT_KHR_opengl_es_enable(_) _(KHR_opengl_es_enable, KHR_OPENGL_ES_ENABLE)
#else
#define OXR_EXTENSION_SUPPORT_KHR_opengl_es_enable(_)
#endif


/*
 * XR_KHR_swapchain_usage_input_attachment_bit
 */
#if defined(XR_KHR_swapchain_usage_input_attachment_bit)
#define OXR_HAVE_KHR_swapchain_usage_input_attachment_bit
#define OXR_EXTENSION_SUPPORT_KHR_swapchain_usage_input_attachment_bit(_)                                              \
	_(KHR_swapchain_usage_input_attachment_bit, KHR_SWAPCHAIN_USAGE_INPUT_ATTACHMENT_BIT)
#else
#define OXR_EXTENSION_SUPPORT_KHR_swapchain_usage_input_attachment_bit(_)
#endif


/*
 * XR_KHR_vulkan_enable
 */
#if defined(XR_KHR_vulkan_enable) && defined(XR_USE_GRAPHICS_API_VULKAN)
#define OXR_HAVE_KHR_vulkan_enable
#define OXR_EXTENSION_SUPPORT_KHR_vulkan_enable(_) _(KHR_vulkan_enable, KHR_VULKAN_ENABLE)
#else
#define OXR_EXTENSION_SUPPORT_KHR_vulkan_enable(_)
#endif


/*
 * XR_KHR_vulkan_enable2
 */
#if defined(XR_KHR_vulkan_enable2) && defined(XR_USE_GRAPHICS_API_VULKAN)
#define OXR_HAVE_KHR_vulkan_enable2
#define OXR_EXTENSION_SUPPORT_KHR_vulkan_enable2(_) _(KHR_vulkan_enable2, KHR_VULKAN_ENABLE2)
#else
#define OXR_EXTENSION_SUPPORT_KHR_vulkan_enable2(_)
#endif


/*
 * XR_KHR_win32_convert_performance_counter_time
 */
#if defined(XR_KHR_win32_convert_performance_counter_time) && defined(XR_USE_PLATFORM_WIN32)
#define OXR_HAVE_KHR_win32_convert_performance_counter_time
#define OXR_EXTENSION_SUPPORT_KHR_win32_convert_performance_counter_time(_)                                            \
	_(KHR_win32_convert_performance_counter_time, KHR_WIN32_CONVERT_PERFORMANCE_COUNTER_TIME)
#else
#define OXR_EXTENSION_SUPPORT_KHR_win32_convert_performance_counter_time(_)
#endif


/*
 * XR_EXT_debug_utils
 */
#if defined(XR_EXT_debug_utils) && defined(XRT_FEATURE_OPENXR_DEBUG_UTILS)
#define OXR_HAVE_EXT_debug_utils
#define OXR_EXTENSION_SUPPORT_EXT_debug_utils(_) _(EXT_debug_utils, EXT_DEBUG_UTILS)
#else
#define OXR_EXTENSION_SUPPORT_EXT_debug_utils(_)
#endif


/*
 * XR_EXT_dpad_binding
 */
#if defined(XR_EXT_dpad_binding)
#define OXR_HAVE_EXT_dpad_binding
#define OXR_EXTENSION_SUPPORT_EXT_dpad_binding(_) _(EXT_dpad_binding, EXT_DPAD_BINDING)
#else
#define OXR_EXTENSION_SUPPORT_EXT_dpad_binding(_)
#endif


/*
 * XR_EXT_hand_tracking
 */
#if defined(XR_EXT_hand_tracking)
#define OXR_HAVE_EXT_hand_tracking
#define OXR_EXTENSION_SUPPORT_EXT_hand_tracking(_) _(EXT_hand_tracking, EXT_HAND_TRACKING)
#else
#define OXR_EXTENSION_SUPPORT_EXT_hand_tracking(_)
#endif


/*
 * XR_FB_display_refresh_rate
 */
#if defined(XR_FB_display_refresh_rate)
#define OXR_HAVE_FB_display_refresh_rate
#define OXR_EXTENSION_SUPPORT_FB_display_refresh_rate(_) _(FB_display_refresh_rate, FB_DISPLAY_REFRESH_RATE)
#else
#define OXR_EXTENSION_SUPPORT_FB_display_refresh_rate(_)
#endif


/*
 * XR_MND_headless
 */
#if defined(XR_MND_headless)
#define OXR_HAVE_MND_headless
#define OXR_EXTENSION_SUPPORT_MND_headless(_) _(MND_headless, MND_HEADLESS)
#else
#define OXR_EXTENSION_SUPPORT_MND_headless(_)
#endif


/*
 * XR_MND_swapchain_usage_input_attachment_bit
 */
#if defined(XR_MND_swapchain_usage_input_attachment_bit)
#define OXR_HAVE_MND_swapchain_usage_input_attachment_bit
#define OXR_EXTENSION_SUPPORT_MND_swapchain_usage_input_attachment_bit(_)                                              \
	_(MND_swapchain_usage_input_attachment_bit, MND_SWAPCHAIN_USAGE_INPUT_ATTACHMENT_BIT)
#else
#define OXR_EXTENSION_SUPPORT_MND_swapchain_usage_input_attachment_bit(_)
#endif


/*
 * XR_EXTX_overlay
 */
#if defined(XR_EXTX_overlay)
#define OXR_HAVE_EXTX_overlay
#define OXR_EXTENSION_SUPPORT_EXTX_overlay(_) _(EXTX_overlay, EXTX_OVERLAY)
#else
#define OXR_EXTENSION_SUPPORT_EXTX_overlay(_)
#endif


/*
 * XR_MNDX_ball_on_a_stick_controller
 */
#if defined(XR_MNDX_ball_on_a_stick_controller)
#define OXR_HAVE_MNDX_ball_on_a_stick_controller
#define OXR_EXTENSION_SUPPORT_MNDX_ball_on_a_stick_controller(_)                                                       \
	_(MNDX_ball_on_a_stick_controller, MNDX_BALL_ON_A_STICK_CONTROLLER)
#else
#define OXR_EXTENSION_SUPPORT_MNDX_ball_on_a_stick_controller(_)
#endif


/*
 * XR_MNDX_egl_enable
 */
#if defined(XR_MNDX_egl_enable) && defined(XR_USE_PLATFORM_EGL) && defined(XR_USE_GRAPHICS_API_OPENGL)
#define OXR_HAVE_MNDX_egl_enable
#define OXR_EXTENSION_SUPPORT_MNDX_egl_enable(_) _(MNDX_egl_enable, MNDX_EGL_ENABLE)
#else
#define OXR_EXTENSION_SUPPORT_MNDX_egl_enable(_)
#endif


/*
 * XR_MNDX_force_feedback_curl
 */
#if defined(XR_MNDX_force_feedback_curl)
#define OXR_HAVE_MNDX_force_feedback_curl
#define OXR_EXTENSION_SUPPORT_MNDX_force_feedback_curl(_) _(MNDX_force_feedback_curl, MNDX_FORCE_FEEDBACK_CURL)
#else
#define OXR_EXTENSION_SUPPORT_MNDX_force_feedback_curl(_)
#endif

// end of GENERATED per-extension defines - do not modify - used by scripts

/*!
 * Call this, passing a macro taking two parameters, to
 * generate tables, code, etc. related to OpenXR extensions.
 * Upon including invoking OXR_EXTENSION_SUPPORT_GENERATE() with some
 * MY_HANDLE_EXTENSION(mixed_case, all_caps), MY_HANDLE_EXTENSION will be called
 * for each extension implemented in Monado and supported in this build:
 *
 * - The first parameter is the name of the extension without the leading XR_
 *   prefix: e.g. `KHR_opengl_enable`
 * - The second parameter is the same text as the first, but in all uppercase,
 *   since this transform cannot be done in the C preprocessor, and some
 *   extension-related entities use this instead of the exact extension name.
 *
 * Implementation note: This macro calls another macro for each extension: that
 * macro is either defined to call your provided macro, or defined to nothing,
 * depending on if the extension is supported in this build.
 *
 * @note Do not edit anything between `clang-format off` and `clang-format on` -
 * it will be replaced next time this file is generated!
 */
// clang-format off
#define OXR_EXTENSION_SUPPORT_GENERATE(_) \
    OXR_EXTENSION_SUPPORT_KHR_android_create_instance(_) \
    OXR_EXTENSION_SUPPORT_KHR_binding_modification(_) \
    OXR_EXTENSION_SUPPORT_KHR_composition_layer_cube(_) \
    OXR_EXTENSION_SUPPORT_KHR_composition_layer_cylinder(_) \
    OXR_EXTENSION_SUPPORT_KHR_composition_layer_depth(_) \
    OXR_EXTENSION_SUPPORT_KHR_composition_layer_equirect(_) \
    OXR_EXTENSION_SUPPORT_KHR_composition_layer_equirect2(_) \
    OXR_EXTENSION_SUPPORT_KHR_convert_timespec_time(_) \
    OXR_EXTENSION_SUPPORT_KHR_D3D11_enable(_) \
    OXR_EXTENSION_SUPPORT_KHR_D3D12_enable(_) \
    OXR_EXTENSION_SUPPORT_KHR_loader_init(_) \
    OXR_EXTENSION_SUPPORT_KHR_loader_init_android(_) \
    OXR_EXTENSION_SUPPORT_KHR_opengl_enable(_) \
    OXR_EXTENSION_SUPPORT_KHR_opengl_es_enable(_) \
    OXR_EXTENSION_SUPPORT_KHR_swapchain_usage_input_attachment_bit(_) \
    OXR_EXTENSION_SUPPORT_KHR_vulkan_enable(_) \
    OXR_EXTENSION_SUPPORT_KHR_vulkan_enable2(_) \
    OXR_EXTENSION_SUPPORT_KHR_win32_convert_performance_counter_time(_) \
    OXR_EXTENSION_SUPPORT_EXT_debug_utils(_) \
    OXR_EXTENSION_SUPPORT_EXT_dpad_binding(_) \
    OXR_EXTENSION_SUPPORT_EXT_hand_tracking(_) \
    OXR_EXTENSION_SUPPORT_FB_display_refresh_rate(_) \
    OXR_EXTENSION_SUPPORT_MND_headless(_) \
    OXR_EXTENSION_SUPPORT_MND_swapchain_usage_input_attachment_bit(_) \
    OXR_EXTENSION_SUPPORT_EXTX_overlay(_) \
    OXR_EXTENSION_SUPPORT_MNDX_ball_on_a_stick_controller(_) \
    OXR_EXTENSION_SUPPORT_MNDX_egl_enable(_) \
    OXR_EXTENSION_SUPPORT_MNDX_force_feedback_curl(_)
// clang-format on
