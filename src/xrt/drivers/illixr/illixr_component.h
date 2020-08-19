#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void* illixr_monado_create_plugin(void* pb);
struct xrt_pose illixr_read_pose();

void illixr_write_frame(unsigned int left,
                        unsigned int right);
int64_t illixr_get_vsync_ns();
int64_t illixr_get_now_ns();
void get_illixr_context();

#ifdef __cplusplus
}
#endif
