# Copyright 2019-2022, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

add_library(
	aux_ogl_gl STATIC
	ogl_documentation.h
	ogl_api.c
	ogl_api.h
	ogl_helpers.c
	ogl_helpers.h
	)
target_link_libraries(aux_ogl_gl PUBLIC aux-includes_gl xrt-external-glad_gl)

if(XRT_HAVE_OPENGL_GLX AND XRT_HAVE_XLIB)
	target_sources(aux_ogl_gl PRIVATE glx_api.c glx_api.h)
endif()

if(XRT_HAVE_EGL)
	target_sources(aux_ogl_gl PRIVATE egl_api.c egl_api.h)
endif()

if(XRT_HAVE_OPENGLES)
	target_link_libraries(aux_ogl_gl PUBLIC EGL::EGL)
endif()

if(WIN32)
	target_sources(aux_ogl_gl PRIVATE wgl_api.c wgl_api.h)
	target_link_libraries(aux_ogl_gl PUBLIC OpenGL::GL)
endif()
