// Copyright 2020, University of Illinois
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Prober code for Illixr.
 * @author Fang Lu <fanglu2@illinois.edu>
 * @ingroup st_prober
 */

#include "util/u_debug.h"
#include "util/u_misc.h"
#include "p_prober.h"

#include "illixr/illixr_interface.h"

#include <stdio.h>
#include <string.h>

/*
 *	Illixr is not an actual hardware device.
 */

// add_device copied from p_prober.c
static void
add_device(struct prober *p, struct prober_device **out_dev)
{
	U_ARRAY_REALLOC_OR_FREE(p->devices, struct prober_device,
	                        (p->num_devices + 1));

	struct prober_device *dev = &p->devices[p->num_devices++];
	U_ZERO(dev);

	*out_dev = dev;
}

int
p_illixr_init(struct prober *p)
{
	return 0;
}

void
p_illixr_teardown(struct prober *p)
{
}

int
p_illixr_probe(struct prober *p)
{
	struct prober_device *pdev;

	const char *illixr_path, *illixr_comp;
	illixr_path = getenv("ILLIXR_PATH");
	illixr_comp = getenv("ILLIXR_COMP");
	if (!illixr_path || !illixr_comp) {
		return 0;
	}

	add_device(p, &pdev);
	pdev->base.vendor_id = ILLIXR_VID;
	pdev->base.product_id = ILLIXR_PID;
	pdev->base.bus = XRT_BUS_TYPE_ILLIXR;
	pdev->illixr.path = illixr_path;
	pdev->illixr.comp = illixr_comp;
	return 0;
}
