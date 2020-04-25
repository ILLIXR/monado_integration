#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void *illixr_monado_create_component(void *);
struct xrt_pose illixr_read_pose();

void illixr_write_frame(unsigned int left, 
                        unsigned int right);
void get_illixr_context();

#ifdef __cplusplus
}
#endif
