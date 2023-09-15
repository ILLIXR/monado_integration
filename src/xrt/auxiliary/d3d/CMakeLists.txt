# Copyright 2019-2022, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

add_library(aux_d3d_gl STATIC d3d_dxgi_formats.h d3d_dxgi_helpers.cpp d3d_dxgi_helpers.hpp)
target_link_libraries(aux_d3d_gl PUBLIC aux-includes_gl xrt-interfaces_gl ${DXGI_LIBRARY} WIL::WIL)

# needed for format includes
target_include_directories(aux_d3d_gl PUBLIC ${Vulkan_INCLUDE_DIR})

if(XRT_HAVE_D3D11)
	target_sources(
		aux_d3d_gl
		PRIVATE
			d3d_d3d11_allocator.cpp
			d3d_d3d11_allocator.h
			d3d_d3d11_allocator.hpp
			d3d_d3d11_bits.h
			d3d_d3d11_fence.cpp
			d3d_d3d11_fence.hpp
			d3d_d3d11_helpers.cpp
			d3d_d3d11_helpers.hpp
		)
	target_link_libraries(aux_d3d_gl PUBLIC ${D3D11_LIBRARY})
endif()

if(XRT_HAVE_D3D12)
	target_sources(
		aux_d3d_gl
		PRIVATE
			d3d_d3d12_bits.h
			# d3d_d3d12_allocator.cpp
			# d3d_d3d12_allocator.hpp
			d3d_d3d12_fence.cpp
			d3d_d3d12_fence.hpp
			d3d_d3d12_helpers.cpp
			d3d_d3d12_helpers.hpp
		)
	target_link_libraries(aux_d3d_gl PUBLIC ${D3D12_LIBRARY})
endif()
