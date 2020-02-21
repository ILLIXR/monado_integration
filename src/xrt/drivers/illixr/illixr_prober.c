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

	return illixr_hmd_create();
}

struct xrt_auto_prober *
illixr_create_auto_prober()
{
	struct illixr_prober *dp = U_TYPED_CALLOC(struct illixr_prober);
	dp->base.destroy = illixr_prober_destroy;
	dp->base.lelo_dallas_autoprobe = illixr_prober_autoprobe;

	return &dp->base;
}
