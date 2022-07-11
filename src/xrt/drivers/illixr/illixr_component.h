#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "GL/gl.h"

void* illixr_monado_create_plugin(void* pb);
struct xrt_pose illixr_read_pose();

void illixr_publish_gl_image_handle(GLuint handle, uint32_t num_images, uint32_t swapchain_index);
void illixr_publish_vk_image_handle(int fd, int64_t format, size_t size, uint32_t width, uint32_t height, uint32_t num_images, uint32_t swapchain_index);

void illixr_write_frame(unsigned int left,
                        unsigned int right);
int64_t illixr_get_vsync_ns();
int64_t illixr_get_now_ns();
void get_illixr_context();

#ifdef __cplusplus
}
#endif
