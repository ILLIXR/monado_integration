// Copyright 2020, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Client side wrapper of instance.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup ipc_client
 */

#include "xrt/xrt_instance.h"
#include "xrt/xrt_gfx_native.h"
#include "xrt/xrt_handles.h"
#include "xrt/xrt_config_os.h"
#include "xrt/xrt_config_android.h"

#include "util/u_misc.h"
#include "util/u_var.h"
#include "util/u_debug.h"
#include "util/u_git_tag.h"
#include "util/u_system_helpers.h"

#include "shared/ipc_protocol.h"
#include "client/ipc_client.h"
#include "ipc_client_generated.h"
#include "util/u_file.h"

#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#ifdef XRT_GRAPHICS_BUFFER_HANDLE_IS_AHARDWAREBUFFER
#include "android/android_ahardwarebuffer_allocator.h"
#endif

#ifdef XRT_OS_ANDROID
#include "android/android_globals.h"
#include "android/ipc_client_android.h"
#endif // XRT_OS_ANDROID

DEBUG_GET_ONCE_LOG_OPTION(ipc_log, "IPC_LOG", U_LOGGING_WARN)
DEBUG_GET_ONCE_BOOL_OPTION(ipc_ignore_version, "IPC_IGNORE_VERSION", false)

/*
 *
 * Struct and helpers.
 *
 */

/*!
 * @implements xrt_instance
 */
struct ipc_client_instance
{
	//! @public Base
	struct xrt_instance base;

	struct ipc_connection ipc_c;

	struct xrt_tracking_origin *xtracks[XRT_SYSTEM_MAX_DEVICES];
	size_t xtrack_count;

	struct xrt_device *xdevs[XRT_SYSTEM_MAX_DEVICES];
	size_t xdev_count;
};

static inline struct ipc_client_instance *
ipc_client_instance(struct xrt_instance *xinst)
{
	return (struct ipc_client_instance *)xinst;
}

#ifdef XRT_OS_ANDROID

static bool
ipc_connect(struct ipc_connection *ipc_c)
{
	ipc_c->log_level = debug_get_log_option_ipc_log();

	ipc_c->ica = ipc_client_android_create(android_globals_get_vm(), android_globals_get_activity());

	if (ipc_c->ica == NULL) {
		IPC_ERROR(ipc_c, "Client create error!");
		return false;
	}

	int socket = ipc_client_android_blocking_connect(ipc_c->ica);
	if (socket < 0) {
		IPC_ERROR(ipc_c, "Service Connect error!");
		return false;
	}
	// The ownership belongs to the Java object. Dup because the fd will be
	// closed when client destroy.
	socket = dup(socket);
	if (socket < 0) {
		IPC_ERROR(ipc_c, "Failed to dup fd with error %d!", errno);
		return false;
	}

	ipc_c->imc.socket_fd = socket;
	ipc_c->imc.log_level = ipc_c->log_level;

	return true;
}


#else
static bool
ipc_connect(struct ipc_connection *ipc_c)
{
	struct sockaddr_un addr;
	int ret;

	ipc_c->log_level = debug_get_log_option_ipc_log();

	// create our IPC socket

	ret = socket(PF_UNIX, SOCK_STREAM, 0);
	if (ret < 0) {
		IPC_ERROR(ipc_c, "Socket Create Error!");
		return false;
	}

	int socket = ret;

	char sock_file[PATH_MAX];

	int size = u_file_get_path_in_runtime_dir(IPC_MSG_SOCK_FILE, sock_file, PATH_MAX);
	if (size == -1) {
		IPC_ERROR(ipc_c, "Could not get socket file name");
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, sock_file);

	ret = connect(socket, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		IPC_ERROR(ipc_c, "Failed to connect to socket %s: %s!", sock_file, strerror(errno));
		close(socket);
		return false;
	}

	ipc_c->imc.socket_fd = socket;
	ipc_c->imc.log_level = ipc_c->log_level;

	return true;
}
#endif

static xrt_result_t
create_system_compositor(struct ipc_client_instance *ii,
                         struct xrt_device *xdev,
                         struct xrt_system_compositor **out_xsysc)
{
	struct xrt_system_compositor *xsysc = NULL;
	struct xrt_image_native_allocator *xina = NULL;

#ifdef XRT_GRAPHICS_BUFFER_HANDLE_IS_AHARDWAREBUFFER
	// On Android, we allocate images natively on the client side.
	xina = android_ahardwarebuffer_allocator_create();
#endif // XRT_GRAPHICS_BUFFER_HANDLE_IS_AHARDWAREBUFFER

	int ret = ipc_client_create_system_compositor(&ii->ipc_c, xina, xdev, &xsysc);
	if (ret < 0 || xsysc == NULL) {
		xrt_images_destroy(&xina);
		return XRT_ERROR_IPC_FAILURE;
	}

	*out_xsysc = xsysc;

	return 0;
}


/*
 *
 * Member functions.
 *
 */

static xrt_result_t
ipc_client_instance_create_system(struct xrt_instance *xinst,
                                  struct xrt_system_devices **out_xsysd,
                                  struct xrt_system_compositor **out_xsysc)
{
	struct ipc_client_instance *ii = ipc_client_instance(xinst);
	xrt_result_t xret = XRT_SUCCESS;

	assert(out_xsysd != NULL);
	assert(*out_xsysd == NULL);
	assert(out_xsysc == NULL || *out_xsysc == NULL);

	// Allocate a helper u_system_devices struct.
	struct u_system_devices *usysd = u_system_devices_allocate();

	// Take the devices from this instance.
	for (uint32_t i = 0; i < ii->xdev_count; i++) {
		usysd->base.xdevs[i] = ii->xdevs[i];
		ii->xdevs[i] = NULL;
	}
	usysd->base.xdev_count = ii->xdev_count;
	ii->xdev_count = 0;

#define SET_ROLE(ROLE)                                                                                                 \
	usysd->base.roles.ROLE = ii->ipc_c.ism->roles.ROLE >= 0 ? usysd->base.xdevs[ii->ipc_c.ism->roles.ROLE] : NULL;

	SET_ROLE(head);
	SET_ROLE(left);
	SET_ROLE(right);
	SET_ROLE(gamepad);
	SET_ROLE(hand_tracking.left);
	SET_ROLE(hand_tracking.right);

#undef SET_ROLE

	// Done here now.
	if (out_xsysc == NULL) {
		*out_xsysd = &usysd->base;
		return XRT_SUCCESS;
	}

	if (usysd->base.roles.head == NULL) {
		IPC_ERROR((&ii->ipc_c), "No head device found but asking for system compositor!");
		u_system_devices_destroy(&usysd);
		return XRT_ERROR_IPC_FAILURE;
	}

	struct xrt_system_compositor *xsysc = NULL;
	xret = create_system_compositor(ii, usysd->base.roles.head, &xsysc);
	if (xret != XRT_SUCCESS) {
		u_system_devices_destroy(&usysd);
		return xret;
	}

	*out_xsysd = &usysd->base;
	*out_xsysc = xsysc;

	return XRT_SUCCESS;
}

static int
ipc_client_instance_get_prober(struct xrt_instance *xinst, struct xrt_prober **out_xp)
{
	*out_xp = NULL;

	return -1;
}

static void
ipc_client_instance_destroy(struct xrt_instance *xinst)
{
	struct ipc_client_instance *ii = ipc_client_instance(xinst);

	// service considers us to be connected until fd is closed
	ipc_message_channel_close(&ii->ipc_c.imc);

	for (size_t i = 0; i < ii->xtrack_count; i++) {
		u_var_remove_root(ii->xtracks[i]);
		free(ii->xtracks[i]);
		ii->xtracks[i] = NULL;
	}
	ii->xtrack_count = 0;

	os_mutex_destroy(&ii->ipc_c.mutex);

#ifdef XRT_OS_ANDROID
	ipc_client_android_destroy(&(ii->ipc_c.ica));
#endif

	free(ii);
}


/*
 *
 * Exported function(s).
 *
 */

/*!
 * Constructor for xrt_instance IPC client proxy.
 *
 * @public @memberof ipc_instance
 */
xrt_result_t
ipc_instance_create(struct xrt_instance_info *i_info, struct xrt_instance **out_xinst)
{
	struct ipc_client_instance *ii = U_TYPED_CALLOC(struct ipc_client_instance);
	ii->base.create_system = ipc_client_instance_create_system;
	ii->base.get_prober = ipc_client_instance_get_prober;
	ii->base.destroy = ipc_client_instance_destroy;

	// FDs needs to be set to something negative.
	ii->ipc_c.imc.socket_fd = -1;
	ii->ipc_c.ism_handle = XRT_SHMEM_HANDLE_INVALID;

	os_mutex_init(&ii->ipc_c.mutex);

	if (!ipc_connect(&ii->ipc_c)) {
		IPC_ERROR((&ii->ipc_c),
		          "Failed to connect to monado service process\n\n"
		          "###\n"
		          "#\n"
		          "# Please make sure that the service process is running\n"
		          "#\n"
		          "# It is called \"monado-service\"\n"
		          "# For builds it's located "
		          "\"build-dir/src/xrt/targets/service/monado-service\"\n"
		          "#\n"
		          "###");
		free(ii);
		return XRT_ERROR_IPC_FAILURE;
	}

	// get our xdev shm from the server and mmap it
	xrt_result_t xret = ipc_call_instance_get_shm_fd(&ii->ipc_c, &ii->ipc_c.ism_handle, 1);
	if (xret != XRT_SUCCESS) {
		IPC_ERROR((&ii->ipc_c), "Failed to retrieve shm fd!");
		free(ii);
		return xret;
	}

	struct ipc_app_state desc = {0};
	desc.info = *i_info;
	desc.pid = getpid(); // Extra info.

	xret = ipc_call_system_set_client_info(&ii->ipc_c, &desc);
	if (xret != XRT_SUCCESS) {
		IPC_ERROR((&ii->ipc_c), "Failed to set instance info!");
		free(ii);
		return xret;
	}

	const int flags = MAP_SHARED;
	const int access = PROT_READ | PROT_WRITE;
	const size_t size = sizeof(struct ipc_shared_memory);

	ii->ipc_c.ism = mmap(NULL, size, access, flags, ii->ipc_c.ism_handle, 0);
	if (ii->ipc_c.ism == NULL) {
		IPC_ERROR((&ii->ipc_c), "Failed to mmap shm!");
		free(ii);
		return XRT_ERROR_IPC_FAILURE;
	}

	if (strncmp(u_git_tag, ii->ipc_c.ism->u_git_tag, IPC_VERSION_NAME_LEN) != 0) {
		IPC_ERROR((&ii->ipc_c), "Monado client library version %s does not match service version %s", u_git_tag,
		          ii->ipc_c.ism->u_git_tag);
		if (!debug_get_bool_option_ipc_ignore_version()) {
			IPC_ERROR((&ii->ipc_c), "Set IPC_IGNORE_VERSION=1 to ignore this version conflict");
			free(ii);
			return XRT_ERROR_IPC_FAILURE;
		}
	}

	uint32_t count = 0;
	struct xrt_tracking_origin *xtrack = NULL;
	struct ipc_shared_memory *ism = ii->ipc_c.ism;

	// Query the server for how many tracking origins it has.
	count = 0;
	for (uint32_t i = 0; i < ism->itrack_count; i++) {
		xtrack = U_TYPED_CALLOC(struct xrt_tracking_origin);

		memcpy(xtrack->name, ism->itracks[i].name, sizeof(xtrack->name));

		xtrack->type = ism->itracks[i].type;
		xtrack->offset = ism->itracks[i].offset;
		ii->xtracks[count++] = xtrack;

		u_var_add_root(xtrack, "Tracking origin", true);
		u_var_add_ro_text(xtrack, xtrack->name, "name");
		u_var_add_pose(xtrack, &xtrack->offset, "offset");
	}

	ii->xtrack_count = count;

	// Query the server for how many devices it has.
	count = 0;
	for (uint32_t i = 0; i < ism->isdev_count; i++) {
		struct ipc_shared_device *isdev = &ism->isdevs[i];
		xtrack = ii->xtracks[isdev->tracking_origin_index];

		if (isdev->name == XRT_DEVICE_GENERIC_HMD) {
			ii->xdevs[count++] = ipc_client_hmd_create(&ii->ipc_c, xtrack, i);
		} else {
			ii->xdevs[count++] = ipc_client_device_create(&ii->ipc_c, xtrack, i);
		}
	}

	ii->xdev_count = count;

	ii->base.startup_timestamp = ii->ipc_c.ism->startup_timestamp;

	*out_xinst = &ii->base;

	return XRT_SUCCESS;
}
