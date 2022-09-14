#pragma once

#include "vk/vk_helpers.h"

#define MESH_WIDTH 512
#define MESH_HEIGHT 512

struct comp_warp_vertex
{
	float position[3];
	float uv[2];
};

struct comp_warp_ubo_data
{
	struct xrt_matrix_4x4 u_renderInverseP;
	struct xrt_matrix_4x4 u_renderInverseV;
	struct xrt_matrix_4x4 u_warpVP;
	float bleedRadius;
	float edgeTolerance;
};

struct comp_warp
{
	struct vk_bundle *vk;

	struct
	{
		VkImage image;
		VkDeviceMemory memory;
		VkImageView view;
		VkSampler sampler;
		VkFramebuffer handle;
	} framebuffers[2];

	VkRenderPass render_pass;

	VkExtent2D extent;

	VkSampleCountFlagBits sample_count;

	VkShaderModule shader_modules[2];
	VkPipeline pipeline_meshwarp;
	VkDescriptorSetLayout descriptor_set_layout_meshwarp;

	VkPipelineLayout pipeline_layout;
	VkPipelineCache pipeline_cache;

	struct xrt_matrix_4x4 mat_world_view[2];
	struct xrt_matrix_4x4 mat_eye_view[2];
	struct xrt_matrix_4x4 mat_projection[2];

	struct vk_buffer vertex_buffer_meshwarp_vertices;
	struct vk_buffer vertex_buffer_meshwarp_indices;

	void* vertices;
	void* indices;

	float nearZ;
	float farZ;

	uint32_t meshwarp_ubo_binding;
	uint32_t meshwarp_texture_binding;
	uint32_t meshwarp_depth_binding;
};
