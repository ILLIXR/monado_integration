#include "math/m_api.h"
#include "main/comp_warp_renderer.h"
#include "render/render_interface.h"

#define C(c)                                                                                                           \
	do {                                                                                                           \
		VkResult ret = c;                                                                                      \
		if (ret != VK_SUCCESS) {                                                                               \
			return false;                                                                                  \
		}                                                                                                      \
	} while (false)

#define D(TYPE, thing)                                                                                                 \
	if (thing != VK_NULL_HANDLE) {                                                                                 \
		vk->vkDestroy##TYPE(vk->device, thing, NULL);                                                          \
		thing = VK_NULL_HANDLE;                                                                                \
	}

#define DD(pool, thing)                                                                                                \
	if (thing != VK_NULL_HANDLE) {                                                                                 \
		free_descriptor_set(vk, pool, thing);                                                                  \
		thing = VK_NULL_HANDLE;                                                                                \
	}

static VkResult
create_descriptor_pool(struct vk_bundle *vk,
                       uint32_t num_uniform_per_desc,
                       uint32_t num_sampler_per_desc,
                       uint32_t num_descs,
                       VkDescriptorPool *out_descriptor_pool)
{
	VkResult ret;


	VkDescriptorPoolSize pool_sizes[2] = {
	    {
	        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        .descriptorCount = num_uniform_per_desc * num_descs,
	    },
	    {
	        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	        .descriptorCount = num_sampler_per_desc * num_descs,
	    },
	};

	VkDescriptorPoolCreateInfo descriptor_pool_info = {
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
	    .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
	    .maxSets = num_descs,
	    .poolSizeCount = ARRAY_SIZE(pool_sizes),
	    .pPoolSizes = pool_sizes,
	};

	VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
	ret = vk->vkCreateDescriptorPool(vk->device,            //
	                                 &descriptor_pool_info, //
	                                 NULL,                  //
	                                 &descriptor_pool);     //
	if (ret != VK_SUCCESS) {
		VK_ERROR(vk, "vkCreateDescriptorPool failed: %s", vk_result_string(ret));
		return ret;
	}

	*out_descriptor_pool = descriptor_pool;

	return VK_SUCCESS;
}

static VkResult
create_render_pass(struct vk_bundle *vk, VkRenderPass *out_renderpass)
{
	VkResult ret;

	VkAttachmentDescription attachments[2] = {
	    {
	        .format = VK_FORMAT_B8G8R8A8_SRGB,
	        .samples = VK_SAMPLE_COUNT_1_BIT,
	        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
	        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
	        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
	        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
	        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	    },
	    {
	        .format = VK_FORMAT_D32_SFLOAT,
	        .samples = VK_SAMPLE_COUNT_1_BIT,
	        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
	        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
	        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
	        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
	        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	    },
	};

	VkAttachmentReference color_attachment = {
	    .attachment = 0,
	    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkAttachmentReference depth_attachment = {
	    .attachment = 1,
	    .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription subpasses[1] = {
	    {
	        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
	        .inputAttachmentCount = 0,
	        .pInputAttachments = NULL,
	        .colorAttachmentCount = 1,
	        .pColorAttachments = &color_attachment,
	        .pResolveAttachments = NULL,
	        .pDepthStencilAttachment = &depth_attachment,
	        .preserveAttachmentCount = 0,
	        .pPreserveAttachments = NULL,
	    },
	};

	VkRenderPassCreateInfo render_pass_info = {
	    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
	    .attachmentCount = ARRAY_SIZE(attachments),
	    .pAttachments = attachments,
	    .subpassCount = ARRAY_SIZE(subpasses),
	    .pSubpasses = subpasses,
	    .dependencyCount = 0,
	    .pDependencies = NULL,
	};


	VkRenderPass render_pass = VK_NULL_HANDLE;
	ret = vk->vkCreateRenderPass(vk->device, &render_pass_info, NULL, &render_pass);
	if (ret != VK_SUCCESS) {
		VK_ERROR(vk, "vkCreateRenderPass failed: %s", vk_result_string(ret));
		return ret;
	}

	*out_renderpass = render_pass;

	return VK_SUCCESS;
}

static void
begin_render_pass(struct vk_bundle *vk,
                  VkCommandBuffer command_buffer,
                  VkRenderPass render_pass,
                  VkFramebuffer framebuffer,
                  VkExtent2D extent)
{
	VkClearValue clear_values[2] = {
	    {.color = {.float32 = {0.3f, 0.3f, 0.3f, 1.0f}}},
	    {.depthStencil = {.depth = 1.0f, .stencil = 0}},
	};

	VkRenderPassBeginInfo render_pass_begin_info = {
	    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
	    .renderPass = render_pass,
	    .framebuffer = framebuffer,
	    .renderArea =
	        {
	            .offset =
	                {
	                    .x = 0,
	                    .y = 0,
	                },
	            .extent =
	                {
	                    .width = extent.width,
	                    .height = extent.height,
	                },
	        },
	    .clearValueCount = ARRAY_SIZE(clear_values),
	    .pClearValues = clear_values,
	};

	vk->vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

static VkResult
create_sampler(struct vk_bundle *vk, VkFilter filter, VkSamplerMipmapMode mipmapmode, VkSampler *out_sampler)
{
	VkSampler sampler;
	VkResult ret;

	VkSamplerCreateInfo info = {
	    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
	    .magFilter = VK_FILTER_LINEAR,
	    .minFilter = filter,
	    .mipmapMode = mipmapmode,
	    .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	    .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	    .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
	    .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
	    .minLod = -1000,
	    .maxLod = 1000,
	    .unnormalizedCoordinates = VK_FALSE,
	};

	ret = vk->vkCreateSampler(vk->device, &info, NULL, &sampler);
	if (ret != VK_SUCCESS) {
		VK_ERROR(vk, "vkCreateSampler: %s", vk_result_string(ret));
		return ret;
	}

	*out_sampler = sampler;

	return VK_SUCCESS;
}

VkResult
create_image(struct vk_bundle *vk,
             VkExtent2D extent,
             VkFormat format,
             VkImageUsageFlags usage,
             VkDeviceMemory *out_mem,
             VkImage *out_image)
{
	VkImageCreateInfo image_info = {
	    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
	    .imageType = VK_IMAGE_TYPE_2D,
	    .format = format,
	    .extent =
	        {
	            .width = extent.width,
	            .height = extent.height,
	            .depth = 1,
	        },
	    .mipLevels = 1,
	    .arrayLayers = 1,
	    .samples = VK_SAMPLE_COUNT_1_BIT,
	    .tiling = VK_IMAGE_TILING_OPTIMAL,
	    .usage = usage,
	    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	    .queueFamilyIndexCount = 0,
	    .pQueueFamilyIndices = NULL,
	    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	VkImage image;
	VkResult ret = vk->vkCreateImage(vk->device, &image_info, NULL, &image);
	if (ret != VK_SUCCESS) {
		VK_ERROR(vk, "vkCreateImage: %s", vk_result_string(ret));
		return ret;
	}

	ret = vk_alloc_and_bind_image_memory(vk, image, SIZE_MAX, "comp_warp_renderer", NULL, out_mem, NULL);
	if (ret != VK_SUCCESS) {
		vk->vkDestroyImage(vk->device, image, NULL);
		return ret;
	}

	*out_image = image;
	return ret;
}

static VkResult
create_framebuffer(struct vk_bundle *vk,
                   VkRenderPass *render_pass,
                   VkImage *color_image,
                   VkImageView *color_view,
                   VkDeviceMemory *color_memory,
                   VkImage *depth_image,
                   VkImageView *depth_view,
                   VkDeviceMemory *depth_memory,
                   VkSampler *texture_sampler,
                   VkSampler *depth_sampler,
                   VkExtent2D extent,
                   VkFramebuffer *out_external_framebuffer)
{
	VkResult ret;

	ret = create_image(vk, extent, VK_FORMAT_B8G8R8A8_SRGB,
	                   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, color_memory, color_image);
	vk_check_error("create_image", ret, false);

	ret = create_image(vk, extent, VK_FORMAT_D32_SFLOAT,
	                   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, depth_memory,
	                   depth_image);
	vk_check_error("create_image", ret, false);

	ret = create_sampler(vk, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, texture_sampler);
	vk_check_error("create_sampler", ret, false);

	ret = create_sampler(vk, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST, depth_sampler);
	vk_check_error("create_sampler", ret, false);

	VkImageSubresourceRange color_subresource_range = {
	    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	    .baseMipLevel = 0,
	    .levelCount = 1,
	    .baseArrayLayer = 0,
	    .layerCount = 1,
	};

	ret = vk_create_view(vk, *color_image, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_FORMAT_B8G8R8A8_SRGB, color_subresource_range, color_view);
	vk_check_error("vk_create_view", ret, false);

	VkImageSubresourceRange depth_subresource_range = {
	    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
	    .baseMipLevel = 0,
	    .levelCount = 1,
	    .baseArrayLayer = 0,
	    .layerCount = 1,
	};

	ret = vk_create_view(vk, *depth_image, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_FORMAT_D32_SFLOAT, depth_subresource_range, depth_view);
	vk_check_error("vk_create_view", ret, false);

	VkImageView attachments[2] = {*color_view, *depth_view};

	VkFramebufferCreateInfo frame_buffer_info = {
	    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
	    .renderPass = *render_pass,
	    .attachmentCount = ARRAY_SIZE(attachments),
	    .pAttachments = attachments,
	    .width = extent.width,
	    .height = extent.height,
	    .layers = 1,
	};

	VkFramebuffer framebuffer = VK_NULL_HANDLE;
	ret = vk->vkCreateFramebuffer(vk->device,         //
	                              &frame_buffer_info, //
	                              NULL,               //
	                              &framebuffer);      //
	if (ret != VK_SUCCESS) {
		VK_ERROR(vk, "vkCreateFramebuffer failed: %s", vk_result_string(ret));
		return ret;
	}

	*out_external_framebuffer = framebuffer;

	return VK_SUCCESS;
}

static VkResult
create_descriptor_set(struct vk_bundle *vk,
                      VkDescriptorPool descriptor_pool,
                      VkDescriptorSetLayout descriptor_layout,
                      VkDescriptorSet *out_descriptor_set)
{
	VkResult ret;

	VkDescriptorSetAllocateInfo alloc_info = {
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
	    .descriptorPool = descriptor_pool,
	    .descriptorSetCount = 1,
	    .pSetLayouts = &descriptor_layout,
	};

	VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
	ret = vk->vkAllocateDescriptorSets(vk->device,       //
	                                   &alloc_info,      //
	                                   &descriptor_set); //
	if (ret != VK_SUCCESS) {
		VK_DEBUG(vk, "vkAllocateDescriptorSets failed: %s", vk_result_string(ret));
		return ret;
	}

	*out_descriptor_set = descriptor_set;

	return VK_SUCCESS;
}

static void
free_descriptor_set(struct vk_bundle *vk, VkDescriptorPool descriptor_pool, VkDescriptorSet descriptor_set)
{
	VkResult ret;

	ret = vk->vkFreeDescriptorSets(vk->device,       //
	                               descriptor_pool,  // descriptorPool
	                               1,                // descriptorSetCount
	                               &descriptor_set); // pDescriptorSets
	if (ret != VK_SUCCESS) {
		VK_DEBUG(vk, "vkFreeDescriptorSets failed: %s", vk_result_string(ret));
	}
}

static VkResult
create_descriptor_set_layout(struct vk_bundle *vk,
                             uint32_t ubo_binding,
                             uint32_t texture_binding,
                             uint32_t depth_binding,
                             VkDescriptorSetLayout *out_descriptor_set_layout)
{
	VkResult ret;

	VkDescriptorSetLayoutBinding set_layout_bindings[3] = {
	    {
	        .binding = texture_binding,
	        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	        .descriptorCount = 1,
	        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	    },
	    {
	        .binding = depth_binding,
	        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	        .descriptorCount = 1,
	        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	    },
	    {
	        .binding = ubo_binding,
	        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        .descriptorCount = 1,
	        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
	    },
	};

	VkDescriptorSetLayoutCreateInfo set_layout_info = {
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
	    .bindingCount = ARRAY_SIZE(set_layout_bindings),
	    .pBindings = set_layout_bindings,
	};

	VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
	ret = vk->vkCreateDescriptorSetLayout(vk->device,              //
	                                      &set_layout_info,        //
	                                      NULL,                    //
	                                      &descriptor_set_layout); //
	if (ret != VK_SUCCESS) {
		VK_ERROR(vk, "vkCreateDescriptorSetLayout failed: %s", vk_result_string(ret));
		return ret;
	}

	*out_descriptor_set_layout = descriptor_set_layout;

	return VK_SUCCESS;
}


static bool
create_vertex_buffers(
    struct vk_bundle *vk, struct render_buffer *vbo, struct render_buffer *ibo, void *vertices, void *indices)
{
	VkBufferUsageFlags vbo_usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	VkBufferUsageFlags ibo_usage_flags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	VkMemoryPropertyFlags memory_property_flags =
	    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

	// vbo and ibo sizes.
	VkDeviceSize vbo_size = sizeof(struct comp_warp_vertex) * VERTICES_SIZE;
	VkDeviceSize ibo_size = sizeof(uint32_t) * INDICES_SIZE;

	C(render_buffer_init(vk,                    // vk_bundle
	                     vbo,                   // buffer
	                     vbo_usage_flags,       // usage_flags
	                     memory_property_flags, // memory_property_flags
	                     vbo_size));            // size

	C(render_buffer_write(vk,         // vk_bundle
	                      vbo,        // buffer
	                      vertices,   // data
	                      vbo_size)); // size

	C(render_buffer_init(vk,                    // vk_bundle
	                     ibo,                   // buffer
	                     ibo_usage_flags,       // usage_flags
	                     memory_property_flags, // memory_property_flags
	                     ibo_size));            // size

	C(render_buffer_write(vk,         // vk_bundle
	                     ibo,        // buffer
	                     indices,    // data
	                     ibo_size)); // size

	return true;
}

static void
create_vertices(struct comp_warp_vertex *vertices)
{
	for (size_t y = 0; y < MESH_HEIGHT + 1; y++) {
		for (size_t x = 0; x < MESH_WIDTH + 1; x++) {

			size_t index = y * (MESH_WIDTH + 1) + x;

			vertices[index].uv[0] = ((float)x / MESH_WIDTH);
			vertices[index].uv[1] = (((MESH_HEIGHT - (float)y) / MESH_HEIGHT));

			if (x == 0) {
				vertices[index].uv[0] = -0.5f;
			}
			if (x == MESH_WIDTH) {
				vertices[index].uv[0] = 1.5f;
			}

			if (y == 0) {
				vertices[index].uv[1] = 1.5f;
			}
			if (y == MESH_HEIGHT) {
				vertices[index].uv[1] = -0.5f;
			}
		}
	}
}

static void
create_indices(uint32_t *indices)
{
	for (size_t y = 0; y < MESH_HEIGHT; y++) {
		for (size_t x = 0; x < MESH_WIDTH; x++) {

			const int offset = (y * MESH_WIDTH + x) * 6;

			indices[offset + 0] = (uint32_t)((y + 0) * (MESH_WIDTH + 1) + (x + 0));
			indices[offset + 1] = (uint32_t)((y + 1) * (MESH_WIDTH + 1) + (x + 0));
			indices[offset + 2] = (uint32_t)((y + 0) * (MESH_WIDTH + 1) + (x + 1));

			indices[offset + 3] = (uint32_t)((y + 0) * (MESH_WIDTH + 1) + (x + 1));
			indices[offset + 4] = (uint32_t)((y + 1) * (MESH_WIDTH + 1) + (x + 0));
			indices[offset + 5] = (uint32_t)((y + 1) * (MESH_WIDTH + 1) + (x + 1));
		}
	}
}

static bool
create_ubo_buffers(struct vk_bundle *vk, struct render_buffer *l_ubo, struct render_buffer *r_ubo)
{
	// Using the same flags for all ubos.
	VkBufferUsageFlags ubo_usage_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	VkMemoryPropertyFlags memory_property_flags =
	    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

	VkDeviceSize ubo_size = sizeof(struct comp_warp_ubo_data);

	C(render_buffer_init(vk, l_ubo, ubo_usage_flags, memory_property_flags, ubo_size));
	C(render_buffer_map(vk, l_ubo));

	C(render_buffer_init(vk, r_ubo, ubo_usage_flags, memory_property_flags, ubo_size));
	C(render_buffer_map(vk, r_ubo));

	return true;
}

static VkResult
create_pipeline_cache(struct vk_bundle *vk, VkPipelineCache *out_pipeline_cache)
{
	VkResult ret;

	VkPipelineCacheCreateInfo pipeline_cache_info = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
	};

	VkPipelineCache pipeline_cache;
	ret = vk->vkCreatePipelineCache(vk->device,           //
	                                &pipeline_cache_info, //
	                                NULL,                 //
	                                &pipeline_cache);     //
	if (ret != VK_SUCCESS) {
		VK_ERROR(vk, "vkCreatePipelineCache failed: %s", vk_result_string(ret));
		return ret;
	}

	*out_pipeline_cache = pipeline_cache;

	return VK_SUCCESS;
}

static VkResult
create_pipeline_layout(struct vk_bundle *vk,
                       VkDescriptorSetLayout descriptor_set_layout,
                       VkPipelineLayout *out_pipeline_layout)
{
	VkResult ret;

	VkPipelineLayoutCreateInfo pipeline_layout_info = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
	    .setLayoutCount = 1,
	    .pSetLayouts = &descriptor_set_layout,
	};

	VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
	ret = vk->vkCreatePipelineLayout(vk->device,            //
	                                 &pipeline_layout_info, //
	                                 NULL,                  //
	                                 &pipeline_layout);     //
	if (ret != VK_SUCCESS) {
		VK_ERROR(vk, "vkCreatePipelineLayout failed: %s", vk_result_string(ret));
		return ret;
	}

	*out_pipeline_layout = pipeline_layout;

	return VK_SUCCESS;
}

static VkResult
create_pipeline(struct vk_bundle *vk,
                VkRenderPass render_pass,
                VkPipelineLayout pipeline_layout,
                VkPipelineCache pipeline_cache,
                uint32_t src_binding,
                VkShaderModule shader_vert,
                VkShaderModule shader_frag,
                VkPipeline *out_mesh_pipeline)
{
	VkResult ret;

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
	    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	    .primitiveRestartEnable = VK_FALSE,
	};

	VkPipelineRasterizationStateCreateInfo rasterization_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
	    .depthClampEnable = VK_FALSE,
	    .rasterizerDiscardEnable = VK_FALSE,
	    .polygonMode = VK_POLYGON_MODE_FILL,
	    .cullMode = VK_CULL_MODE_NONE,
	    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
	    .lineWidth = 1.0f,
	};

	VkPipelineColorBlendAttachmentState blend_attachment_state = {
	    .blendEnable = VK_FALSE,
	    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
	                      VK_COLOR_COMPONENT_A_BIT,
	    .srcColorBlendFactor = VK_BLEND_FACTOR_ONE, // this is what layer uses
	    .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
	    .colorBlendOp = VK_BLEND_OP_ADD,
	    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
	    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
	    .alphaBlendOp = VK_BLEND_OP_ADD,
	};

	VkPipelineColorBlendStateCreateInfo color_blend_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
	    .attachmentCount = 1,
	    .blendConstants = {0, 0, 0, 0},
	    .pAttachments = &blend_attachment_state,
	};

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
	    .depthTestEnable = VK_TRUE,
	    .depthWriteEnable = VK_TRUE,
	    .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
	};

	VkPipelineViewportStateCreateInfo viewport_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
	    .viewportCount = 1,
	    .scissorCount = 1,
	};

	VkPipelineMultisampleStateCreateInfo multisample_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
	    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	    .minSampleShading = 0.0f,
	    .pSampleMask = VK_FALSE,
	    .alphaToCoverageEnable = VK_FALSE,
	};

	VkDynamicState dynamic_states[] = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR,
	};

	VkPipelineDynamicStateCreateInfo dynamic_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
	    .dynamicStateCount = ARRAY_SIZE(dynamic_states),
	    .pDynamicStates = dynamic_states,
	};

	VkVertexInputAttributeDescription vertex_input_attribute_descriptions[2] = {
	    {
	        .binding = src_binding,
	        .location = 0,
	        .format = VK_FORMAT_R32G32B32_SFLOAT,
	        .offset = offsetof(struct comp_warp_vertex, position),
	    },
	    {
	        .binding = src_binding,
	        .location = 1,
	        .format = VK_FORMAT_R32G32_SFLOAT,
	        .offset = offsetof(struct comp_warp_vertex, uv),
	    },
	};

	VkVertexInputBindingDescription vertex_input_binding_description[1] = {
	    {.binding = src_binding,
	     .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	     .stride = sizeof(struct comp_warp_vertex)},
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	    .vertexAttributeDescriptionCount = ARRAY_SIZE(vertex_input_attribute_descriptions),
	    .pVertexAttributeDescriptions = vertex_input_attribute_descriptions,
	    .vertexBindingDescriptionCount = ARRAY_SIZE(vertex_input_binding_description),
	    .pVertexBindingDescriptions = vertex_input_binding_description,
	};

	VkPipelineShaderStageCreateInfo shader_stages[2] = {
	    {
	        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	        .stage = VK_SHADER_STAGE_VERTEX_BIT,
	        .module = shader_vert,
	        .pName = "main",
	    },
	    {
	        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
	        .module = shader_frag,
	        .pName = "main",
	    },
	};

	VkGraphicsPipelineCreateInfo pipeline_info = {
	    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
	    .stageCount = ARRAY_SIZE(shader_stages),
	    .pStages = shader_stages,
	    .pVertexInputState = &vertex_input_state,
	    .pInputAssemblyState = &input_assembly_state,
	    .pViewportState = &viewport_state,
	    .pRasterizationState = &rasterization_state,
	    .pMultisampleState = &multisample_state,
	    .pDepthStencilState = &depth_stencil_state,
	    .pColorBlendState = &color_blend_state,
	    .pDynamicState = &dynamic_state,
	    .layout = pipeline_layout,
	    .renderPass = render_pass,
	    .basePipelineHandle = VK_NULL_HANDLE,
	    .basePipelineIndex = -1,
	};

	VkPipeline pipeline = VK_NULL_HANDLE;
	ret = vk->vkCreateGraphicsPipelines(vk->device,     //
	                                    pipeline_cache, //
	                                    1,              //
	                                    &pipeline_info, //
	                                    NULL,           //
	                                    &pipeline);     //
	if (ret != VK_SUCCESS) {
		VK_DEBUG(vk, "vkCreateGraphicsPipelines failed: %s", vk_result_string(ret));
		return ret;
	}

	*out_mesh_pipeline = pipeline;

	return VK_SUCCESS;
}

bool
init(struct comp_warp_renderer *c, struct vk_bundle *vk, struct render_shaders *s, VkExtent2D extent)
{
	c->vk = *vk;

	c->nearZ = 0.1;
	c->farZ = 100.0f;

	c->ubo_binding = 0;
	c->src_binding = 0;
	c->color_binding = 1;
	c->depth_binding = 2;

	c->extent = extent;

	C(create_descriptor_pool(vk, 1, 2, 16 * 2, &c->descriptor_pool));

	C(create_descriptor_set_layout(vk, c->ubo_binding, c->color_binding, c->depth_binding,
	                               &c->descriptor_set_layout));

	C(create_descriptor_set(vk, c->descriptor_pool, c->descriptor_set_layout, &c->views[0].descriptor_set));
	C(create_descriptor_set(vk, c->descriptor_pool, c->descriptor_set_layout, &c->views[1].descriptor_set));

	C(create_render_pass(vk, &c->render_pass));

	C(create_pipeline_cache(vk, &c->pipeline_cache));
	C(create_pipeline_layout(vk, c->descriptor_set_layout, &c->pipeline_layout));
	C(create_pipeline(vk, c->render_pass, c->pipeline_layout, c->pipeline_cache, c->src_binding,
	                  s->openwarp_mesh_vert, s->openwarp_mesh_frag, &c->pipeline));

	create_vertices(c->vertices);
	create_indices(c->indices);

	if (!create_vertex_buffers(vk, &c->vbo, &c->ibo, c->vertices, c->indices)) {
		return false;
	}

	if (!create_ubo_buffers(vk, &c->views[0].ubo, &c->views[1].ubo)) {
		return false;
	}

	for (uint32_t i = 0; i < 2; i++) {
		math_matrix_4x4_identity(&c->mat_projection[i]);
		math_matrix_4x4_identity(&c->mat_render[i]);
		math_matrix_4x4_identity(&c->mat_world_view[i]);
		math_matrix_4x4_identity(&c->mat_eye_view[i]);

		C(create_framebuffer(vk, &c->render_pass, &c->framebuffers[i].color_image,
		                     &c->framebuffers[i].color_view, &c->framebuffers[i].color_memory,
		                     &c->framebuffers[i].depth_image, &c->framebuffers[i].depth_view,
		                     &c->framebuffers[i].depth_memory, &c->framebuffers[i].texture_sampler,
		                     &c->framebuffers[i].depth_sampler, extent, &c->framebuffers[i].handle));

		comp_warp_update_descriptor_set(
			&c->vk, c->color_binding, c->depth_binding, c->framebuffers[i].texture_sampler, c->framebuffers[i].depth_sampler,
			c->framebuffers[i].color_view,
			c->framebuffers[i].depth_view, c->ubo_binding,
			c->views[i].ubo.buffer, VK_WHOLE_SIZE, c->views[i].descriptor_set);
	}

	return true;
}

struct comp_warp_renderer *
comp_warp_create(struct vk_bundle *vk, struct render_shaders *s, VkExtent2D extent)
{
	struct comp_warp_renderer *c = U_TYPED_CALLOC(struct comp_warp_renderer);
	init(c, vk, s, extent);
	return c;
}

void
comp_reproject(struct comp_warp_renderer *c)
{
	struct vk_bundle *vk = &c->vk;

	VkCommandBuffer cmd_buffer;
	if (vk_cmd_buffer_create(vk, &cmd_buffer) != VK_SUCCESS) {
		return;
	}

	// ---------------

	os_mutex_lock(&vk->cmd_pool_mutex);

	VkViewport viewport = {
	    .x = 0,
	    .y = 0,
	    .width = c->extent.width,
	    .height = c->extent.height,
	    .minDepth = 0,
	    .maxDepth = 1.0f,
	};
	vk->vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);

	VkRect2D scissor = {
	    .offset = {0, 0},
	    .extent = c->extent,
	};
	vk->vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);

	for (uint32_t i = 0; i < 2; i++) {
		begin_render_pass(vk, cmd_buffer, c->render_pass, c->framebuffers[i].handle, c->extent);

		struct xrt_matrix_4x4 u_warpVP;
		struct xrt_matrix_4x4 u_renderInverseP;
		math_matrix_4x4_inverse(&c->mat_projection[i], &u_renderInverseP);
		math_matrix_4x4_multiply(&c->mat_projection[i], &c->mat_world_view[i], &u_warpVP);

		// update UBO struct
		c->ubo_data[i].u_renderInverseP = u_renderInverseP;
		c->ubo_data[i].u_renderInverseV = c->mat_render[i];
		c->ubo_data[i].u_warpVP = u_warpVP;
		c->ubo_data[i].bleedRadius = 1.0f / MESH_WIDTH;
		c->ubo_data[i].edgeTolerance = 0.0001f;

		render_buffer_write(vk, &c->views[i].ubo, &c->ubo_data[i], sizeof(struct comp_warp_ubo_data));

		vk->vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, c->pipeline_layout, 0, 1,
		                            &c->views[i].descriptor_set, 0, NULL);

		vk->vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, c->pipeline);

		VkDeviceSize offsets[1] = {0};
		vk->vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &c->vbo.buffer, offsets);
		vk->vkCmdBindIndexBuffer(cmd_buffer, c->ibo.buffer, 0, VK_INDEX_TYPE_UINT32);

		vk->vkCmdDrawIndexed(cmd_buffer, INDICES_SIZE, 1, 0, 0, 0);

		vk->vkCmdEndRenderPass(cmd_buffer);
	}

	os_mutex_unlock(&vk->cmd_pool_mutex);

	// ---------------

	VkResult res = vk_cmd_buffer_submit(vk, cmd_buffer);
	vk_check_error("vk_submit_cmd_buffer", res, );
}

void
comp_warp_set_pose(struct comp_warp_renderer *c,
                   const struct xrt_pose *eye_pose,
                   const struct xrt_pose *world_pose,
                   uint32_t eye)
{
	math_matrix_4x4_view_from_pose(eye_pose, &c->mat_eye_view[eye]);
	math_matrix_4x4_view_from_pose(world_pose, &c->mat_world_view[eye]);
}

void
comp_warp_set_projection(struct comp_warp_renderer *self, const struct xrt_fov *fov, uint32_t eye)
{
	const float tan_left = tanf(fov->angle_left);
	const float tan_right = tanf(fov->angle_right);

	const float tan_down = tanf(fov->angle_down);
	const float tan_up = tanf(fov->angle_up);

	const float tan_width = tan_right - tan_left;
	const float tan_height = tan_up - tan_down;

	const float a11 = 2 / tan_width;
	const float a22 = 2 / tan_height;

	const float a31 = (tan_right + tan_left) / tan_width;
	const float a32 = (tan_up + tan_down) / tan_height;
	const float a33 = -(self->farZ + self->nearZ) / (self->farZ - self->nearZ);

	const float a43 = -(2 * self->farZ * self->nearZ) / (self->farZ - self->nearZ);

	// clang-format off
	self->mat_projection[eye] = (struct xrt_matrix_4x4) {
		.v = {
			a11, 0,   0,   0,
			0,   a22, 0,   0,
			a31, a32, a33, -1,
			0,   0,   a43,  0,
		}
	};
	// clang-format on
}

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
                                VkDescriptorSet descriptor_set)
{
	VkDescriptorImageInfo texture_image_info = {
	    .sampler = texture_sampler,
	    .imageView = texture_image_view,
	    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};

	VkDescriptorImageInfo depth_image_info = {
	    .sampler = depth_sampler,
	    .imageView = depth_image_view,
	    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};

	VkDescriptorBufferInfo buffer_info = {
	    .buffer = buffer,
	    .offset = 0,
	    .range = size,
	};

	VkWriteDescriptorSet write_descriptor_sets[3] = {
	    {
	        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	        .dstSet = descriptor_set,
	        .dstBinding = texture_binding,
	        .descriptorCount = 1,
	        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	        .pImageInfo = &texture_image_info,
	    },
	    {
	        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	        .dstSet = descriptor_set,
	        .dstBinding = depth_binding,
	        .descriptorCount = 1,
	        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	        .pImageInfo = &depth_image_info,
	    },
	    {
	        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	        .dstSet = descriptor_set,
	        .dstBinding = ubo_binding,
	        .descriptorCount = 1,
	        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        .pBufferInfo = &buffer_info,
	    },
	};

	vk->vkUpdateDescriptorSets(vk->device, ARRAY_SIZE(write_descriptor_sets), write_descriptor_sets, 0, NULL);
}

void
comp_warp_destroy(struct comp_warp_renderer *c)
{
	struct vk_bundle *vk = &c->vk;

	D(RenderPass, c->render_pass);
	D(Pipeline, c->pipeline);
	D(PipelineLayout, c->pipeline_layout);
	D(Framebuffer, c->framebuffers[0].handle);
	D(Framebuffer, c->framebuffers[1].handle);
	render_buffer_close(vk, &c->views[0].ubo);
	render_buffer_close(vk, &c->views[1].ubo);
	DD(c->descriptor_pool, c->views[0].descriptor_set);
	DD(c->descriptor_pool, c->views[1].descriptor_set);

	U_ZERO(c);
}
