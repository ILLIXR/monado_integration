#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void *illixr_monado_create_component(void *);
struct xrt_pose illixr_read_pose();

#ifdef __cplusplus
}
#endif
