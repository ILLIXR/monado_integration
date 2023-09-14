// Copyright 2019-2020, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Very simple file opening functions.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Pete Black <pblack@collabora.com>
 * @ingroup aux_util
 */

#include "xrt/xrt_config_os.h"
#include "util/u_file.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


#ifdef XRT_OS_LINUX
#include <linux/limits.h>

static int
mkpath(const char *path)
{
	char tmp[PATH_MAX];
	char *p = NULL;
	size_t len;

	snprintf(tmp, sizeof(tmp), "%s", path);
	len = strlen(tmp) - 1;
	if (tmp[len] == '/') {
		tmp[len] = 0;
	}

	for (p = tmp + 1; *p; p++) {
		if (*p == '/') {
			*p = 0;
			if (mkdir(tmp, S_IRWXU) < 0 && errno != EEXIST) {
				return -1;
			}
			*p = '/';
		}
	}

	if (mkdir(tmp, S_IRWXU) < 0 && errno != EEXIST) {
		return -1;
	}

	return 0;
}

ssize_t
u_file_get_config_dir(char *out_path, size_t out_path_size)
{
	const char *xdg_home = getenv("XDG_CONFIG_HOME");
	const char *home = getenv("HOME");
	if (xdg_home != NULL) {
		return snprintf(out_path, out_path_size, "%s/monado", xdg_home);
	}
	if (home != NULL) {
		return snprintf(out_path, out_path_size, "%s/.config/monado", home);
	}
	return -1;
}

ssize_t
u_file_get_path_in_config_dir(const char *suffix, char *out_path, size_t out_path_size)
{
	char tmp[PATH_MAX];
	ssize_t i = u_file_get_config_dir(tmp, sizeof(tmp));
	if (i <= 0) {
		return -1;
	}

	return snprintf(out_path, out_path_size, "%s/%s", tmp, suffix);
}

FILE *
u_file_open_file_in_config_dir(const char *filename, const char *mode)
{
	char tmp[PATH_MAX];
	ssize_t i = u_file_get_config_dir(tmp, sizeof(tmp));
	if (i <= 0) {
		return NULL;
	}

	char file_str[PATH_MAX + 15];
	i = snprintf(file_str, sizeof(file_str), "%s/%s", tmp, filename);
	if (i <= 0) {
		return NULL;
	}

	FILE *file = fopen(file_str, mode);
	if (file != NULL) {
		return file;
	}

	// Try creating the path.
	mkpath(tmp);

	// Do not report error.
	return fopen(file_str, mode);
}

ssize_t
u_file_get_runtime_dir(char *out_path, size_t out_path_size)
{
	const char *xgd_rt = getenv("XDG_RUNTIME_DIR");
	if (xgd_rt != NULL) {
		return snprintf(out_path, out_path_size, "%s", xgd_rt);
	}

	const char *tmp = "/tmp";
	return snprintf(out_path, out_path_size, "%s", tmp);
}

ssize_t
u_file_get_path_in_runtime_dir(const char *suffix, char *out_path, size_t out_path_size)
{
	char tmp[PATH_MAX];
	ssize_t i = u_file_get_runtime_dir(tmp, sizeof(tmp));
	if (i <= 0) {
		return -1;
	}

	return snprintf(out_path, out_path_size, "%s/%s", tmp, suffix);
}

#endif

char *
u_file_read_content(FILE *file)
{
	// Go to the end of the file.
	fseek(file, 0L, SEEK_END);
	size_t file_size = ftell(file);

	// Return back to the start of the file.
	fseek(file, 0L, SEEK_SET);

	char *buffer = (char *)calloc(file_size + 1, sizeof(char));
	if (buffer == NULL) {
		return NULL;
	}

	// Do the actual reading.
	size_t ret = fread(buffer, sizeof(char), file_size, file);
	if (ret != file_size) {
		free(buffer);
		return NULL;
	}

	return buffer;
}

char *
u_file_read_content_from_path(const char *path)
{
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		return NULL;
	}
	char *file_content = u_file_read_content(file);
	int ret = fclose(file);
	// We don't care about the return value since we're just reading
	(void)ret;

	// Either valid non-null or null
	return file_content;
}
