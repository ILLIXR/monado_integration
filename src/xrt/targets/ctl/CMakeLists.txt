# Copyright 2020-2021, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

add_executable(monado-ctl_gl main.c)
add_sanitizers(monado-ctl_gl)

target_include_directories(monado-ctl_gl PRIVATE ipc_gl)

target_link_libraries(monado-ctl_gl PRIVATE aux_util_gl ipc_client_gl)

install(TARGETS monado-ctl_gl RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
