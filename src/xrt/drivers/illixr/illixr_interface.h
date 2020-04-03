// Copyright 2020, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Interface to illixr driver.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup drv_illixr
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Placeholder values
#define ILLIXR_VID 0x0114
#define ILLIXR_PID 0x0514

/*!
 * @defgroup drv_illixr illixr driver.
 * @ingroup drv
 *
 * @brief Simple do nothing illixr driver.
 */

/*!
 * Create a auto prober for illixr devices.
 *
 * @ingroup drv_illixr
 */
struct xrt_auto_prober *
illixr_create_auto_prober(void);

/*!
 * Create a illixr hmd.
 *
 * @ingroup drv_illixr
 */
struct xrt_device *
illixr_hmd_create(const char *path, const char *comp);


/*!
 * Probing function for Illixr.
 *
 * @ingroup drv_hydra
 */
int
illixr_found(struct xrt_prober *xp,
             struct xrt_prober_device **devices,
             size_t num_devices,
             size_t index,
             struct xrt_device **out_xdevs);

/*!
 * @dir drivers/illixr
 *
 * @brief @ref drv_illixr files.
 */


#ifdef __cplusplus
}
#endif
