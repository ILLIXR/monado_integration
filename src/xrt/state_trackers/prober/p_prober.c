// Copyright 2019, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Main prober code.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup st_prober
 */

#include "util/u_var.h"
#include "util/u_misc.h"
#include "util/u_debug.h"
#include "p_prober.h"

#ifdef XRT_HAVE_V4L2
#include "v4l2/v4l2_interface.h"
#endif

#include <stdio.h>
#include <string.h>


/*
 *
 * Pre-declare functions.
 *
 */

DEBUG_GET_ONCE_BOOL_OPTION(prober_spew, "PROBER_PRINT_SPEW", false)
DEBUG_GET_ONCE_BOOL_OPTION(prober_debug, "PROBER_PRINT_DEBUG", false)

static void
add_device(struct prober *p, struct prober_device **out_dev);

static int
initialize(struct prober *p, struct xrt_prober_entry_lists *lists);

static void
teardown_devices(struct prober *p);

static void
teardown(struct prober *p);

static int
probe(struct xrt_prober *xp);

static int
dump(struct xrt_prober *xp);

static int
select_device(struct xrt_prober *xp,
              struct xrt_device **xdevs,
              size_t num_xdevs);

static int
open_hid_interface(struct xrt_prober *xp,
                   struct xrt_prober_device *xpdev,
                   int interface,
                   struct os_hid_device **out_hid_dev);

static int
open_video_device(struct xrt_prober *xp,
                  struct xrt_prober_device *xpdev,
                  struct xrt_frame_context *xfctx,
                  struct xrt_fs **out_xfs);

static int
list_video_devices(struct xrt_prober *xp,
                   xrt_prober_list_video_cb cb,
                   void *ptr);
static int
get_string_descriptor(struct xrt_prober *xp,
                      struct xrt_prober_device *xpdev,
                      enum xrt_prober_string which_string,
                      unsigned char *buffer,
                      int length);

static bool
can_open(struct xrt_prober *xp, struct xrt_prober_device *xpdev);

static void
destroy(struct xrt_prober **xp);


/*
 *
 * "Exported" functions.
 *
 */

int
xrt_prober_create_with_lists(struct xrt_prober **out_xp,
                             struct xrt_prober_entry_lists *lists)
{
	struct prober *p = U_TYPED_CALLOC(struct prober);

	int ret = initialize(p, lists);
	if (ret != 0) {
		free(p);
		return ret;
	}

	*out_xp = &p->base;

	return 0;
}

#define ENUM_TO_STR(r)                                                         \
	case r: return #r

const char *
xrt_prober_string_to_string(enum xrt_prober_string t)
{
	switch (t) {
		ENUM_TO_STR(XRT_PROBER_STRING_MANUFACTURER);
		ENUM_TO_STR(XRT_PROBER_STRING_PRODUCT);
		ENUM_TO_STR(XRT_PROBER_STRING_SERIAL_NUMBER);
	}
	return "";
}

const char *
xrt_bus_type_to_string(enum xrt_bus_type t)
{
	switch (t) {
		ENUM_TO_STR(XRT_BUS_TYPE_UNKNOWN);
		ENUM_TO_STR(XRT_BUS_TYPE_USB);
		ENUM_TO_STR(XRT_BUS_TYPE_BLUETOOTH);
	}
	return "";
}

bool
xrt_prober_match_string(struct xrt_prober *xp,
                        struct xrt_prober_device *dev,
                        enum xrt_prober_string type,
                        const char *to_match)
{
	unsigned char s[256] = {0};
	int len = xrt_prober_get_string_descriptor(xp, dev, type, s, sizeof(s));
	if (len == 0)
		return false;

	return 0 == strncmp(to_match, (const char *)s, sizeof(s));
}


int
p_dev_get_usb_dev(struct prober *p,
                  uint16_t bus,
                  uint16_t addr,
                  uint16_t vendor_id,
                  uint16_t product_id,
                  struct prober_device **out_pdev)
{
	struct prober_device *pdev;

	for (size_t i = 0; i < p->num_devices; i++) {
		struct prober_device *pdev = &p->devices[i];

		if (pdev->base.bus != XRT_BUS_TYPE_USB ||
		    pdev->usb.bus != bus || pdev->usb.addr != addr) {
			continue;
		}

		if (pdev->base.vendor_id != vendor_id ||
		    pdev->base.product_id != product_id) {
			P_ERROR(p,
			        "USB device with same address but different "
			        "vendor and product found!\n"
			        "\tvendor:  %04x %04x\n"
			        "\tproduct: %04x %04x",
			        pdev->base.vendor_id, vendor_id,
			        pdev->base.product_id, product_id);
			continue;
		}

		*out_pdev = pdev;
		return 0;
	}

	add_device(p, &pdev);
	pdev->base.vendor_id = vendor_id;
	pdev->base.product_id = product_id;
	pdev->base.bus = XRT_BUS_TYPE_USB;
	pdev->usb.bus = bus;
	pdev->usb.addr = addr;
	*out_pdev = pdev;

	return 0;
}

int
p_dev_get_bluetooth_dev(struct prober *p,
                        uint64_t id,
                        uint16_t vendor_id,
                        uint16_t product_id,
                        struct prober_device **out_pdev)
{
	struct prober_device *pdev;

	for (size_t i = 0; i < p->num_devices; i++) {
		struct prober_device *pdev = &p->devices[i];

		if (pdev->base.bus != XRT_BUS_TYPE_BLUETOOTH ||
		    pdev->bluetooth.id != id) {
			continue;
		}

		if (pdev->base.vendor_id != vendor_id ||
		    pdev->base.product_id != product_id) {
			P_ERROR(p,
			        "Bluetooth device with same address but "
			        "different vendor and product found!\n"
			        "\tvendor:  %04x %04x\n"
			        "\tproduct: %04x %04x",
			        pdev->base.vendor_id, vendor_id,
			        pdev->base.product_id, product_id);
			continue;
		}

		*out_pdev = pdev;
		return 0;
	}

	add_device(p, &pdev);
	pdev->base.vendor_id = vendor_id;
	pdev->base.product_id = product_id;
	pdev->base.bus = XRT_BUS_TYPE_BLUETOOTH;
	pdev->bluetooth.id = id;

	*out_pdev = pdev;

	return 0;
}


/*
 *
 * Internal functions.
 *
 */

static void
fill_out_product(struct prober *p, struct prober_device *pdev)
{
	const char *bus =
	    pdev->base.bus == XRT_BUS_TYPE_BLUETOOTH ? "bluetooth" : "usb";

	char *str = NULL;
	int ret = 0;
	do {
		ret = snprintf(str, ret, "Unknown %s device: %04x:%04x", bus,
		               pdev->base.vendor_id, pdev->base.product_id);
		if (ret <= 0) {
			return;
		}

		if (str == NULL) {
			str = U_CALLOC_WITH_CAST(char, ret + 1);
		} else {
			pdev->usb.product = str;
			return;
		}
	} while (true);
}

static void
add_device(struct prober *p, struct prober_device **out_dev)
{
	U_ARRAY_REALLOC_OR_FREE(p->devices, struct prober_device,
	                        (p->num_devices + 1));

	struct prober_device *dev = &p->devices[p->num_devices++];
	U_ZERO(dev);

	*out_dev = dev;
}

static void
add_usb_entry(struct prober *p, struct xrt_prober_entry *entry)
{
	U_ARRAY_REALLOC_OR_FREE(p->entries, struct xrt_prober_entry *,
	                        (p->num_entries + 1));
	p->entries[p->num_entries++] = entry;
}

static int
collect_entries(struct prober *p)
{
	struct xrt_prober_entry_lists *lists = p->lists;
	while (lists) {
		for (size_t j = 0; lists->entries != NULL && lists->entries[j];
		     j++) {
			struct xrt_prober_entry *entry = lists->entries[j];
			for (size_t k = 0; entry[k].found != NULL; k++) {
				add_usb_entry(p, &entry[k]);
			}
		}

		lists = lists->next;
	}

	return 0;
}

static int
initialize(struct prober *p, struct xrt_prober_entry_lists *lists)
{
	p->base.probe = probe;
	p->base.dump = dump;
	p->base.select = select_device;
	p->base.open_hid_interface = open_hid_interface;
	p->base.open_video_device = open_video_device;
	p->base.list_video_devices = list_video_devices;
	p->base.get_string_descriptor = get_string_descriptor;
	p->base.can_open = can_open;
	p->base.destroy = destroy;
	p->lists = lists;
	p->print_spew = debug_get_bool_option_prober_spew();
	p->print_debug = debug_get_bool_option_prober_debug();

	u_var_add_root((void *)p, "Prober", true);
	u_var_add_bool((void *)p, &p->print_debug, "Debug");
	u_var_add_bool((void *)p, &p->print_spew, "Spew");

	int ret;

	ret = collect_entries(p);
	if (ret != 0) {
		teardown(p);
		return -1;
	}

	ret = p_illixr_init(p);
	if (ret != 0) {
		teardown(p);
		return -1;
	}

#ifdef XRT_HAVE_LIBUSB
	ret = p_libusb_init(p);
	if (ret != 0) {
		teardown(p);
		return -1;
	}
#endif

#ifdef XRT_HAVE_LIBUVC
	ret = p_libuvc_init(p);
	if (ret != 0) {
		teardown(p);
		return -1;
	}
#endif

	ret = p_tracking_init(p);
	if (ret != 0) {
		teardown(p);
		return -1;
	}

	for (int i = 0; i < MAX_AUTO_PROBERS && lists->auto_probers[i]; i++) {
		p->auto_probers[i] = lists->auto_probers[i]();
	}

	return 0;
}

static void
teardown_devices(struct prober *p)
{
	// Need to free all devices.
	for (size_t i = 0; i < p->num_devices; i++) {
		struct prober_device *pdev = &p->devices[i];

#ifdef XRT_OS_LINUX
		if (pdev->usb.product != NULL) {
			free((char *)pdev->usb.product);
			pdev->usb.product = NULL;
		}

		if (pdev->usb.path != NULL) {
			free((char *)pdev->usb.path);
			pdev->usb.path = NULL;
		}

		for (size_t j = 0; j < pdev->num_v4ls; j++) {
			struct prober_v4l *v4l = &pdev->v4ls[j];
			free((char *)v4l->path);
			v4l->path = NULL;
		}

		if (pdev->v4ls != NULL) {
			free(pdev->v4ls);
			pdev->v4ls = NULL;
			pdev->num_v4ls = 0;
		}

		for (size_t j = 0; j < pdev->num_hidraws; j++) {
			struct prober_hidraw *hidraw = &pdev->hidraws[j];
			free((char *)hidraw->path);
			hidraw->path = NULL;
		}

		if (pdev->hidraws != NULL) {
			free(pdev->hidraws);
			pdev->hidraws = NULL;
			pdev->num_hidraws = 0;
		}
#endif
	}

	if (p->devices != NULL) {
		free(p->devices);
		p->devices = NULL;
		p->num_devices = 0;
	}
}

static void
teardown(struct prober *p)
{
	// First remove the variable tracking.
	u_var_remove_root((void *)p);

	// Clean up all auto_probers.
	for (int i = 0; i < MAX_AUTO_PROBERS && p->auto_probers[i]; i++) {
		p->auto_probers[i]->destroy(p->auto_probers[i]);
		p->auto_probers[i] = NULL;
	}

	// Need to turn off tracking early.
	p_tracking_teardown(p);

	// Need to free all entries.
	if (p->entries != NULL) {
		free(p->entries);
		p->entries = NULL;
		p->num_entries = 0;
	}

	teardown_devices(p);

	p_illixr_teardown(p);

#ifdef XRT_HAVE_LIBUVC
	p_libuvc_teardown(p);
#endif

#ifdef XRT_HAVE_LIBUSB
	p_libusb_teardown(p);
#endif
}


/*
 *
 * Member functions.
 *
 */

static int
probe(struct xrt_prober *xp)
{
	struct prober *p = (struct prober *)xp;
	XRT_MAYBE_UNUSED int ret = 0;

	// Free old list first.
	teardown_devices(p);

	ret = p_illixr_probe(p);
	if (ret != 0) {
		P_ERROR(p, "Failed to enumerate illixr devices\n");
		return -1;
	}

#ifdef XRT_HAVE_LIBUDEV
	ret = p_udev_probe(p);
	if (ret != 0) {
		P_ERROR(p, "Failed to enumerate udev devices\n");
		return -1;
	}
#endif

#ifdef XRT_HAVE_LIBUSB
	ret = p_libusb_probe(p);
	if (ret != 0) {
		P_ERROR(p, "Failed to enumerate libusb devices\n");
		return -1;
	}
#endif

#ifdef XRT_HAVE_LIBUVC
	ret = p_libuvc_probe(p);
	if (ret != 0) {
		P_ERROR(p, "Failed to enumerate libuvc devices\n");
		return -1;
	}
#endif

	return 0;
}

static int
dump(struct xrt_prober *xp)
{
	struct prober *p = (struct prober *)xp;
	XRT_MAYBE_UNUSED ssize_t k = 0;
	XRT_MAYBE_UNUSED size_t j = 0;

	for (size_t i = 0; i < p->num_devices; i++) {
		struct prober_device *pdev = &p->devices[i];
		p_dump_device(p, pdev, (int)i);
	}

	return 0;
}

static void
handle_found_device(struct prober *p,
                    struct xrt_device **xdevs,
                    size_t num_xdevs,
                    struct xrt_device *xdev)
{
	P_DEBUG(p, "Found '%s' %p", xdev->str, (void *)xdev);

	// For controllers we put them after the first found HMD.
	if (xdev->hmd == NULL) {
		for (size_t i = 1; i < num_xdevs; i++) {
			if (xdevs[i] == NULL) {
				xdevs[i] = xdev;
				return;
			}
		}

		P_ERROR(p, "Too many controller devices closing '%s'",
		        xdev->str);
		xdev->destroy(xdev);
		return;
	}

	// Not found a HMD before, add it first in the list.
	if (xdevs[0] == NULL) {
		xdevs[0] = xdev;
		return;
	}

	P_ERROR(p, "Found more then one, HMD closing '%s'", xdev->str);
	xdev->destroy(xdev);
}

static int
select_device(struct xrt_prober *xp,
              struct xrt_device **xdevs,
              size_t num_xdevs)
{
	struct prober *p = (struct prober *)xp;

	// Build a list of all current probed devices.
	struct xrt_prober_device **dev_list =
	    U_TYPED_ARRAY_CALLOC(struct xrt_prober_device *, p->num_devices);
	for (size_t i = 0; i < p->num_devices; i++) {
		dev_list[i] = &p->devices[i].base;
	}

	// Loop over all devices and entries that might match them.
	for (size_t i = 0; i < p->num_devices; i++) {
		struct prober_device *pdev = &p->devices[i];

		for (size_t k = 0; k < p->num_entries; k++) {
			struct xrt_prober_entry *entry = p->entries[k];
			if (pdev->base.vendor_id != entry->vendor_id ||
			    pdev->base.product_id != entry->product_id) {
				continue;
			}

			struct xrt_device
			    *new_xdevs[XRT_MAX_DEVICES_PER_PROBE] = {NULL};
			int num_found = entry->found(
			    xp, dev_list, p->num_devices, i, &(new_xdevs[0]));

			if (num_found <= 0) {
				continue;
			}
			for (int created_idx = 0; created_idx < num_found;
			     ++created_idx) {
				if (new_xdevs[created_idx] == NULL) {
					P_DEBUG(
					    p,
					    "Leaving device creation loop "
					    "early: found function reported %i "
					    "created, but only %i non-null",
					    num_found, created_idx);
					continue;
				}
				handle_found_device(p, xdevs, num_xdevs,
				                    new_xdevs[created_idx]);
			}
		}
	}

	// Free the temporary list.
	free(dev_list);

	for (int i = 0; i < MAX_AUTO_PROBERS && p->auto_probers[i]; i++) {

		/*
		 * If we have found a HMD, tell the auto probers not to open
		 * any more HMDs. This is mostly to stop OpenHMD and Monado
		 * fighting over devices.
		 */
		bool no_hmds = xdevs[0] != NULL;

		struct xrt_device *xdev =
		    p->auto_probers[i]->lelo_dallas_autoprobe(
		        p->auto_probers[i], no_hmds, xp);
		if (xdev == NULL) {
			continue;
		}

		handle_found_device(p, xdevs, num_xdevs, xdev);
	}


	if (xdevs[0] != NULL) {
		P_DEBUG(p, "Found HMD! '%s'", xdevs[0]->str);
		return 0;
	}

	P_DEBUG(p, "Didn't find any HMD devices");

	// Destroy all other found devices.
	for (size_t i = 1; i < num_xdevs; i++) {
		if (xdevs[i] == NULL) {
			continue;
		}

		P_DEBUG(p, "Destroying '%s'", xdevs[i]->str);
		xdevs[i]->destroy(xdevs[i]);
		xdevs[i] = NULL;
	}

	return 0;
}

static int
open_hid_interface(struct xrt_prober *xp,
                   struct xrt_prober_device *xpdev,
                   int interface,
                   struct os_hid_device **out_hid_dev)
{
	struct prober_device *pdev = (struct prober_device *)xpdev;
	int ret;

	for (size_t j = 0; j < pdev->num_hidraws; j++) {
		struct prober_hidraw *hidraw = &pdev->hidraws[j];

		if (hidraw->interface != interface) {
			continue;
		}

		ret = os_hid_open_hidraw(hidraw->path, out_hid_dev);
		if (ret != 0) {
			P_ERROR(p, "Failed to open device '%s' got '%i'",
			        hidraw->path, ret);
			return ret;
		}

		return 0;
	}

	P_ERROR(p,
	        "Could not find the requested "
	        "hid interface (%i) on the device!",
	        interface);
	return -1;
}

static int
open_video_device(struct xrt_prober *xp,
                  struct xrt_prober_device *xpdev,
                  struct xrt_frame_context *xfctx,
                  struct xrt_fs **out_xfs)
{
	struct prober_device *pdev = (struct prober_device *)xpdev;

#if defined(XRT_HAVE_V4L2)
	if (pdev->num_v4ls == 0) {
		return -1;
	}

	struct xrt_fs *xfs = v4l2_fs_create(xfctx, pdev->v4ls[0].path);
	if (xfs == NULL) {
		return -1;
	}

	*out_xfs = xfs;
	return 0;
#else
	(void)pdev;
	return -1;
#endif
}

static int
list_video_devices(struct xrt_prober *xp,
                   xrt_prober_list_video_cb cb,
                   void *ptr)
{
	struct prober *p = (struct prober *)xp;

	// Loop over all devices and find video devices.
	for (size_t i = 0; i < p->num_devices; i++) {
		struct prober_device *pdev = &p->devices[i];

		bool has = false;
#ifdef XRT_HAVE_LIBUVC
		has |= pdev->uvc.dev != NULL;
#endif

#ifdef XRT_OS_LINUX
		has |= pdev->num_v4ls > 0;
#endif
		if (!has) {
			continue;
		}

		if (pdev->usb.product == NULL) {
			fill_out_product(p, pdev);
		}

		cb(xp, &pdev->base, pdev->usb.product, ptr);
	}

	return 0;
}

static int
get_string_descriptor(struct xrt_prober *xp,
                      struct xrt_prober_device *xpdev,
                      enum xrt_prober_string which_string,
                      unsigned char *buffer,
                      int length)
{
	struct prober *p = (struct prober *)xp;
	struct prober_device *pdev = (struct prober_device *)xpdev;
	int ret;
#ifdef XRT_HAVE_LIBUSB
	if (pdev->usb.dev != NULL) {
		ret = p_libusb_get_string_descriptor(p, pdev, which_string,
		                                     buffer, length);
		if (ret >= 0) {
			return ret;
		}
	}
#endif
	//! @todo add more backends
	//! @todo make this unicode (utf-16)? utf-8 would be better...
	return 0;
}

static bool
can_open(struct xrt_prober *xp, struct xrt_prober_device *xpdev)
{
	struct prober *p = (struct prober *)xp;
	struct prober_device *pdev = (struct prober_device *)xpdev;
#ifdef XRT_HAVE_LIBUSB
	if (pdev->usb.dev != NULL) {
		return p_libusb_can_open(p, pdev);
	}
#endif
	//! @todo add more backends
	return false;
}


static void
destroy(struct xrt_prober **xp)
{
	struct prober *p = (struct prober *)*xp;
	if (p == NULL) {
		return;
	}

	teardown(p);
	free(p);

	*xp = NULL;
}
