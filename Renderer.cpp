/* @file Renderer.cpp

    Main renderer program that takes charge of rendering.
    SPDX-License-Identifier: WTFPL

*/

#include <Renderer.hpp>
#include <Environment.hpp>
#include <Frontend.hpp>
#include <Shader.hpp>

static VkPipelineLayout vulkanGraphicsPipelineLayout;
static VkPipelineLayout vulkanComputePipelineLayout;
static VkRenderPass vulkanRenderPass;
static VkPipeline vulkanGraphicsPipeline;
static VkPipeline vulkanComputePipeline;
static VkFramebuffer *swapChainFramebuffers;
static VkCommandPool vulkanCommandPool;
static VkCommandBuffer vulkanGraphicsCommandBuffer;
static VkCommandBuffer vulkanComputeCommandBuffer;
static VkSemaphore vulkanImageAvailableSemaphore;
static VkSemaphore vulkanRenderFinishedSemaphore;
static VkFence vulkanInFlightFence;
static VkSampler vulkanComputeResultImageSampler;
static VkImage vulkanComputeResultImage;
static VkImageView vulkanComputeResultImageView;
static VkDeviceMemory vulkanComputeResultImageMemory;
static VkDescriptorSetLayout vulkanComputeDescriptorSetLayout;
static VkDescriptorSetLayout vulkanGraphicsDescriptorSetLayout;
static VkPipelineShaderStageCreateInfo GraphicsShaderStages[2];
static VkPipelineShaderStageCreateInfo ComputeShaderStage;
static VkDescriptorPool vulkanDescriptorPool;
static VkDescriptorSet vulkanComputeDescriptorSet;

static uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vulkanPhysicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if (typeFilter & (1 << i)) {
            return i;
        }
    }
    return -1;
}

enum TransitionFlow {
    TRANSITION_FROM_NULL_TO_COMPUTE,
    TRANSITION_FROM_COMPUTE_TO_GRAPHICS,
    TRANSITION_FROM_GRAPHICS_TO_COMPUTE
};

static VkResult transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, TransitionFlow flow) {

    VkResult result;
    VkCommandBuffer commandBuffer;

    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vulkanCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    result = vkAllocateCommandBuffers(vulkanLogicalDevice, &allocInfo, &commandBuffer);
    if(result != VK_SUCCESS) {
        return result;
    }

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if(result != VK_SUCCESS) {
        return result;
    }

    VkPipelineStageFlagBits srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlagBits dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };

    switch(flow) {
        case TRANSITION_FROM_NULL_TO_COMPUTE: {
            barrier.srcAccessMask = VK_ACCESS_NONE;
            barrier.dstAccessMask = VK_ACCESS_NONE;
            srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            break;
        }
        case TRANSITION_FROM_COMPUTE_TO_GRAPHICS: {
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            break;
        }
        case TRANSITION_FROM_GRAPHICS_TO_COMPUTE: {
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            break;
        }
    }
    vkCmdPipelineBarrier(
        commandBuffer,
        srcStageMask,
        dstStageMask,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    vkEndCommandBuffer(commandBuffer);
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };

    vkQueueSubmit(vulkanComputeQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vulkanComputeQueue);

    vkFreeCommandBuffers(vulkanLogicalDevice, vulkanCommandPool, 1, &commandBuffer);
    return VK_SUCCESS;
}

static VkResult recordGraphicsCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{

    VkResult result;
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (result != VK_SUCCESS) {
        return result;
    }

    VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
    VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = vulkanRenderPass,
        .framebuffer = swapChainFramebuffers[imageIndex],
        .renderArea = {
            .offset = {0, 0},
            .extent = {
                .width = WINDOW_WIDTH,
                .height = WINDOW_HEIGHT
            }
        },
        .clearValueCount = 1,
        .pClearValues = &clearColor
    };

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanGraphicsPipeline);
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanGraphicsPipelineLayout,
        0, 1, &vulkanComputeDescriptorSet, 0, 0);

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(WINDOW_WIDTH),
        .height = static_cast<float>(WINDOW_HEIGHT),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = { 0, 0 },
        .extent = {
            .width = WINDOW_WIDTH,
            .height = WINDOW_HEIGHT
        }
    };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdDraw(commandBuffer, 6, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    return vkEndCommandBuffer(commandBuffer);
}

static VkResult recordComputeCommandBuffer(VkCommandBuffer commandBuffer)
{

    VkResult result;
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (result != VK_SUCCESS) {
        return result;
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkanComputePipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkanComputePipelineLayout,
        0, 1, &vulkanComputeDescriptorSet, 0, 0);

    vkCmdDispatch(commandBuffer, WINDOW_WIDTH / 16, WINDOW_HEIGHT / 16, 1);


    return vkEndCommandBuffer(commandBuffer);
}

// Create pipeline, submit tasks...
VkResult BeginRenderingOperation(void)
{

    VkResult result;
    result = CreateShaderStageFromFile("shader.frag.spv",VK_SHADER_STAGE_FRAGMENT_BIT,&GraphicsShaderStages[0]);
    if (result != VK_SUCCESS) {
        return result;
    }
    result = CreateShaderStageFromFile("shader.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, &GraphicsShaderStages[1]);
    if (result != VK_SUCCESS) {
        return result;
    }
    result = CreateShaderStageFromFile("shader.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT, &ComputeShaderStage);
    if (result != VK_SUCCESS) {
        return result;
    }
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = sizeof(dynamicStates) / sizeof(VkDynamicState),
        .pDynamicStates = dynamicStates
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr, // Optional
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = WINDOW_WIDTH,
        .height = WINDOW_HEIGHT,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VkRect2D scissor = {
        .offset = { 0, 0 },
        .extent = {
            .width = WINDOW_WIDTH,
            .height = WINDOW_HEIGHT
        }
    };

    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f, // Optional
        .pSampleMask = nullptr, // Optional
        .alphaToCoverageEnable = VK_FALSE, // Optional
        .alphaToOneEnable = VK_FALSE // Optional
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment
    };


    VkAttachmentDescription colorAttachment = {
        .format = vulkanSurfaceFormat.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };
    
    VkAttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    };

    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    result = vkCreateRenderPass(vulkanLogicalDevice, &renderPassInfo, nullptr, &vulkanRenderPass);
    if (result != VK_SUCCESS) {
        return result;
    }

    VkFramebufferCreateInfo framebufferInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = vulkanRenderPass,
        .attachmentCount = 1,
        .width = WINDOW_WIDTH,
        .height = WINDOW_HEIGHT,
        .layers = 1,
    };

    swapChainFramebuffers = new VkFramebuffer[vulkanSwapChainImageCount];
    for (uint32_t iter = 0; iter < vulkanSwapChainImageCount; ++iter) {
        framebufferInfo.pAttachments = &vulkanSwapChainImageViews[iter];
        result = vkCreateFramebuffer(vulkanLogicalDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[iter]);
        if (result != VK_SUCCESS) {
            return result;
        }
    }

    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = vulkanGraphicsQueueFamilyIndex,
    };

    result = vkCreateCommandPool(vulkanLogicalDevice, &poolInfo, nullptr, &vulkanCommandPool);
    if (result != VK_SUCCESS) {
        return result;
    }

    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vulkanCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    result = vkAllocateCommandBuffers(vulkanLogicalDevice, &allocInfo, &vulkanGraphicsCommandBuffer);
    if (result != VK_SUCCESS) {
        return result;
    }

    result = vkAllocateCommandBuffers(vulkanLogicalDevice, &allocInfo, &vulkanComputeCommandBuffer);
    if (result != VK_SUCCESS) {
        return result;
    }

    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    if (vkCreateSemaphore(vulkanLogicalDevice, &semaphoreInfo, nullptr, &vulkanImageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(vulkanLogicalDevice, &semaphoreInfo, nullptr, &vulkanRenderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(vulkanLogicalDevice, &fenceInfo, nullptr, &vulkanInFlightFence) != VK_SUCCESS) {
        return VK_ERROR_UNKNOWN;
    }

    VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .extent = {
            .width = WINDOW_WIDTH,
            .height = WINDOW_HEIGHT,
            .depth = 1
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    result = vkCreateImage(vulkanLogicalDevice, &imageInfo, nullptr, &vulkanComputeResultImage);
    if (result != VK_SUCCESS) {
        return result;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(vulkanLogicalDevice, vulkanComputeResultImage, &memRequirements);

    VkMemoryAllocateInfo memoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };

    result = vkAllocateMemory(vulkanLogicalDevice, &memoryAllocateInfo, nullptr, &vulkanComputeResultImageMemory);
    if (result != VK_SUCCESS) {
        return result;
    }
    vkBindImageMemory(vulkanLogicalDevice, vulkanComputeResultImage, vulkanComputeResultImageMemory, 0);

    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding[] = {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        }
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 2,
        .pBindings = descriptorSetLayoutBinding
    };
    result = vkCreateDescriptorSetLayout(vulkanLogicalDevice, &layoutInfo, nullptr, &vulkanComputeDescriptorSetLayout);
    if (result != VK_SUCCESS) {
        return result;
    }

    VkPipelineLayoutCreateInfo graphicsPipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &vulkanComputeDescriptorSetLayout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr

    };

    result = vkCreatePipelineLayout(vulkanLogicalDevice, &graphicsPipelineLayoutInfo, nullptr, &vulkanGraphicsPipelineLayout);
    if (result != VK_SUCCESS) {
        return result;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .stageCount = 2,
        .pStages = GraphicsShaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = nullptr, // Optional
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = vulkanGraphicsPipelineLayout,
        .renderPass = vulkanRenderPass,
        .subpass = 0
    };

    result = vkCreateGraphicsPipelines(vulkanLogicalDevice, VK_NULL_HANDLE, 1,&pipelineInfo, nullptr, &vulkanGraphicsPipeline);
    if (result != VK_SUCCESS) {
        return result;
    }

    vulkanComputePipelineLayout = vulkanGraphicsPipelineLayout;

    VkComputePipelineCreateInfo computePipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = ComputeShaderStage,
        .layout = vulkanComputePipelineLayout,
    };

    result = vkCreateComputePipelines(vulkanLogicalDevice, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &vulkanComputePipeline);
    if (result != VK_SUCCESS) {
        return result;
    }
    
    VkDescriptorPoolSize poolSize[] = {
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1
        }
    };

    VkDescriptorPoolCreateInfo descriptorPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 2,
        .poolSizeCount = 2,
        .pPoolSizes = poolSize
    };
    result = vkCreateDescriptorPool(vulkanLogicalDevice, &descriptorPoolInfo, nullptr, &vulkanDescriptorPool);
    if (result != VK_SUCCESS) {
        return result;
    }

    VkDescriptorSetAllocateInfo descriptorSetallocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = vulkanDescriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &vulkanComputeDescriptorSetLayout
    };
    result = vkAllocateDescriptorSets(vulkanLogicalDevice, &descriptorSetallocInfo, &vulkanComputeDescriptorSet);
    if (result != VK_SUCCESS) {
        return result;
    }

    VkImageViewCreateInfo computeResultViewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = vulkanComputeResultImage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    result = vkCreateImageView(vulkanLogicalDevice, &computeResultViewInfo, nullptr, &vulkanComputeResultImageView);
    if (result != VK_SUCCESS) {
        return result;
    }

    VkSamplerCreateInfo samplerInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE
    };

    result = vkCreateSampler(vulkanLogicalDevice, &samplerInfo, nullptr, &vulkanComputeResultImageSampler);
    if (result != VK_SUCCESS) {
        return result;
    }

    VkDescriptorImageInfo computeImageInfo = {
        .sampler = vulkanComputeResultImageSampler,
        .imageView = vulkanComputeResultImageView,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL
    };

    VkWriteDescriptorSet write[] = {
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = vulkanComputeDescriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &computeImageInfo
        },
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = vulkanComputeDescriptorSet,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &computeImageInfo
        }
    };

    vkUpdateDescriptorSets(vulkanLogicalDevice, 2, write, 0, nullptr);
    
    return transitionImageLayout(vulkanComputeResultImage, VK_FORMAT_R32G32B32A32_SFLOAT,
                                 VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,TRANSITION_FROM_NULL_TO_COMPUTE);

}

VkResult DrawNextFrame(void)
{

    uint32_t imageIndex;
    VkResult result;

    vkWaitForFences(vulkanLogicalDevice, 1, &vulkanInFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(vulkanLogicalDevice, 1, &vulkanInFlightFence);

    result = vkAcquireNextImageKHR(vulkanLogicalDevice, vulkanSwapChain, UINT64_MAX,
                                   vulkanImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    if (result != VK_SUCCESS) {
        // Window resize? We cannot resize window!
        // The only reason is Window is closing!
        // F**k Windows bug!
        return result;
    }
    vkResetCommandBuffer(vulkanGraphicsCommandBuffer, 0);
    recordGraphicsCommandBuffer(vulkanGraphicsCommandBuffer, imageIndex);
    recordComputeCommandBuffer(vulkanComputeCommandBuffer);
    VkSemaphore waitSemaphores[] = { vulkanImageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };

    // First compute, then render.
    VkSubmitInfo computeSubmitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &vulkanImageAvailableSemaphore,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &vulkanComputeCommandBuffer,
    };
    result = vkQueueSubmit(vulkanComputeQueue, 1, &computeSubmitInfo, nullptr);


    VkSubmitInfo graphicsSubmitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &vulkanGraphicsCommandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &vulkanRenderFinishedSemaphore
    };
    result = vkQueueSubmit(vulkanGraphicsQueue, 1, &graphicsSubmitInfo, vulkanInFlightFence);
    if (result != VK_SUCCESS) {
        return result;
    }

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &vulkanRenderFinishedSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &vulkanSwapChain,
        .pImageIndices = &imageIndex
    };

    return vkQueuePresentKHR(vulkanGraphicsQueue, &presentInfo);
}

// End rendering & destroy allocated environments.
VkResult EndRenderingOperation(void)
{
    if (vulkanImageAvailableSemaphore != nullptr) {
        vkDestroySemaphore(vulkanLogicalDevice, vulkanImageAvailableSemaphore, nullptr);
    }
    if (vulkanRenderFinishedSemaphore != nullptr) {
        vkDestroySemaphore(vulkanLogicalDevice, vulkanRenderFinishedSemaphore, nullptr);
    }
    if (vulkanInFlightFence != nullptr) {
        vkDestroyFence(vulkanLogicalDevice, vulkanInFlightFence, nullptr);
    }
    if (vulkanCommandPool != nullptr) {
        vkDestroyCommandPool(vulkanLogicalDevice, vulkanCommandPool, nullptr);
    }
    if (swapChainFramebuffers != nullptr) {
        for (uint32_t iter = 0; iter < vulkanSwapChainImageCount; ++iter) {
            vkDestroyFramebuffer(vulkanLogicalDevice, swapChainFramebuffers[iter], nullptr);
        }
        delete[] swapChainFramebuffers;
    }
    if (vulkanGraphicsPipeline != nullptr) {
        vkDestroyPipeline(vulkanLogicalDevice, vulkanGraphicsPipeline, nullptr);
    }
    if (vulkanComputePipeline != nullptr) {
        vkDestroyPipeline(vulkanLogicalDevice, vulkanComputePipeline, nullptr);
    }
    if (vulkanGraphicsPipelineLayout != nullptr) {
        vkDestroyPipelineLayout(vulkanLogicalDevice, vulkanGraphicsPipelineLayout, nullptr);
    }
    if (vulkanRenderPass != nullptr) {
        vkDestroyRenderPass(vulkanLogicalDevice, vulkanRenderPass, nullptr);
    }
    if (vulkanDescriptorPool != nullptr) {
        vkDestroyDescriptorPool(vulkanLogicalDevice, vulkanDescriptorPool, nullptr);
    }
    if (vulkanComputeDescriptorSetLayout != nullptr) {
        vkDestroyDescriptorSetLayout(vulkanLogicalDevice, vulkanComputeDescriptorSetLayout, nullptr);
    }
    if (vulkanComputeResultImageSampler != nullptr) {
        vkDestroySampler(vulkanLogicalDevice, vulkanComputeResultImageSampler, nullptr);
    }
    if (vulkanComputeResultImageView != nullptr) {
        vkDestroyImageView(vulkanLogicalDevice, vulkanComputeResultImageView, nullptr);
    }
    if (vulkanComputeResultImage != nullptr) {
        vkDestroyImage(vulkanLogicalDevice, vulkanComputeResultImage, nullptr);
    }
    if (vulkanComputeResultImageMemory!= nullptr) {
        vkFreeMemory(vulkanLogicalDevice, vulkanComputeResultImageMemory, nullptr);
    }
    if (GraphicsShaderStages[0].module != nullptr) {
        vkDestroyShaderModule(vulkanLogicalDevice, GraphicsShaderStages[0].module, nullptr);
    }
    if (GraphicsShaderStages[1].module != nullptr) {
        vkDestroyShaderModule(vulkanLogicalDevice, GraphicsShaderStages[1].module, nullptr);
    }
    if (ComputeShaderStage.module != nullptr) {
        vkDestroyShaderModule(vulkanLogicalDevice, ComputeShaderStage.module, nullptr);
    }
    return VK_SUCCESS;
}
