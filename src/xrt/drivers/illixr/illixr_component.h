#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "xrt/xrt_compositor.h"

void* illixr_monado_create_plugin(void* pb);
struct xrt_pose illixr_read_pose();

void illixr_publish_gl_image_handle(unsigned int handle, int num_images, int swapchain_index);
void illixr_publish_vk_image_handle(int fd, uint64_t size, uint64_t format, int width, int height, int num_images, int swapchain_index);

void illixr_write_frame(unsigned int left,
                        unsigned int right);
int64_t illixr_get_vsync_ns();
int64_t illixr_get_now_ns();
void get_illixr_context();

#ifdef __cplusplus
}
#endif
