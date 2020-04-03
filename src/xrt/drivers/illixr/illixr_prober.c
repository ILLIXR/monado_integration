// Copyright 2020, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  illixr prober code.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup drv_ohmd
 */

#include <stdio.h>
#include <stdlib.h>

#include "xrt/xrt_prober.h"

#include "util/u_misc.h"
#include "util/u_debug.h"

#include "illixr_interface.h"


struct illixr_prober
{
	struct xrt_auto_prober base;
};

static inline struct illixr_prober *
illixr_prober(struct xrt_auto_prober *p)
{
	return (struct illixr_prober *)p;
}

static void
illixr_prober_destroy(struct xrt_auto_prober *p)
{
	struct illixr_prober *dp = illixr_prober(p);

	free(dp);
}

static struct xrt_device *
illixr_prober_autoprobe(struct xrt_auto_prober *xap,
                       bool no_hmds,
                       struct xrt_prober *xp)
{
	struct illixr_prober *dp = illixr_prober(xap);
	(void)dp;

	// Do not create a illixr HMD if we are not looking for HMDs.
	if (no_hmds) {
		return NULL;
	}

	const char *illixr_path, *illixr_comp;
	illixr_path = getenv("ILLIXR_PATH");
	illixr_comp = getenv("ILLIXR_COMP");
	if (!illixr_path || !illixr_comp) {
		return NULL;
	}

	return illixr_hmd_create(illixr_path, illixr_comp);
}

struct xrt_auto_prober *
illixr_create_auto_prober()
{
	struct illixr_prober *dp = U_TYPED_CALLOC(struct illixr_prober);
	dp->base.destroy = illixr_prober_destroy;
	dp->base.lelo_dallas_autoprobe = illixr_prober_autoprobe;

	return &dp->base;
}

int
illixr_found(struct xrt_prober *xp,
             struct xrt_prober_device **devices,
             size_t num_devices,
             size_t index,
             struct xrt_device **out_xdevs)
{
	const char *path = xrt_prober_get_illixr_path(xp, devices[index]);
	const char *comp = xrt_prober_get_illixr_components(xp, devices[index]);
	out_xdevs[0] = illixr_hmd_create(path, comp);
	return 1;
}
