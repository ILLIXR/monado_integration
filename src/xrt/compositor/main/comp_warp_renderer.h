#pragma once

#include "vk/vk_helpers.h"
#include "render/render_interface.h"
#include "comp_compositor.h"

#define MESH_WIDTH 1024
#define MESH_HEIGHT 1024
#define VERTICES_SIZE ((MESH_WIDTH + 1) * (MESH_HEIGHT + 1))
#define INDICES_SIZE (2 * 3 * MESH_WIDTH * MESH_HEIGHT)

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

struct comp_warp_renderer
{
	struct vk_bundle vk;

	struct
	{
		struct render_buffer ubo;
		VkDescriptorSet descriptor_set;
	} views[2];

	struct comp_warp_ubo_data ubo_data[2];

	struct xrt_matrix_4x4 mat_render[2];
	struct xrt_matrix_4x4 mat_projection[2];
	struct xrt_matrix_4x4 mat_eye_view[2];
	struct xrt_matrix_4x4 mat_world_view[2];

	struct
	{
		VkImage color_image;
		VkImageView color_view;
		VkDeviceMemory color_memory;
		VkImage depth_image;
		VkImageView depth_view;
		VkDeviceMemory depth_memory;
		VkSampler texture_sampler;
		VkSampler depth_sampler;
		VkFramebuffer handle;
	} framebuffers[2];

	struct render_buffer vbo;
	struct render_buffer ibo;

	VkCommandBuffer cmd;
	VkDescriptorPool descriptor_pool;
	VkDescriptorSetLayout descriptor_set_layout;

	VkDescriptorSetLayout descriptor_set_layout_meshwarp;

	VkRenderPass render_pass;

	VkShaderModule shader_modules[2];

	VkPipelineLayout pipeline_layout;
	VkPipelineCache pipeline_cache;
	VkPipeline pipeline;

	VkExtent2D extent;

	struct comp_warp_vertex vertices[VERTICES_SIZE];
	uint32_t indices[INDICES_SIZE];

	float nearZ;
	float farZ;

	uint32_t ubo_binding;
	uint32_t color_binding;
	uint32_t depth_binding;
	uint32_t src_binding;
};

struct comp_warp_renderer *
comp_warp_create(struct vk_bundle *vk, struct render_shaders *s, VkExtent2D extent);

void
comp_reproject(struct comp_warp_renderer *c);

void
comp_warp_set_pose(struct comp_warp_renderer *c,
                   const struct xrt_pose *eye_pose,
                   const struct xrt_pose *world_pose,
                   uint32_t eye);

void
comp_warp_set_projection(struct comp_warp_renderer *self, const struct xrt_fov *fov, uint32_t eye);

void
comp_warp_update_descriptor_set(struct vk_bundle *vk,
                                uint32_t texture_binding,
                                uint32_t depth_binding,
                                VkSampler texture_sampler,
                                VkSampler depth_sampler,
                                VkImageView texture_image_view,
                                VkImageView depth_image_view,
                                uint32_t ubo_binding,
                                VkBuffer buffer,
                                VkDeviceSize size,
                                VkDescriptorSet descriptor_set);

void
comp_warp_destroy(struct comp_warp_renderer *c);
