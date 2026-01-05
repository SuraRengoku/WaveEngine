#include "VulkanPipeline.h"

namespace  WAVEENGINE::GRAPHICS::VULKAN {

VkResult vulkanPipelineLayout::create(const deviceContext& dCtx, const VkPipelineLayoutCreateInfo& createInfo) {
    assert(!_impl && "::VULKAN:ERROR PipelineLayout already created\n");

    auto impl = std::make_shared<pipelineLayoutImpl>();
    impl->device = dCtx._device;

    if (VkResult result = vkCreatePipelineLayout(
        dCtx._device,
        &createInfo,
        dCtx._allocator,
        &impl->layout
    )) {
        debug_error("::VULKAN:ERROR Failed to create a pipeline layout\n");
        return result;
    }

    _impl = std::move(impl);
    return VK_SUCCESS;
}

VkResult vulkanPipelineLayout::create(const deviceContext& dCtx, u32 setLayoutCount,
    const VkDescriptorSetLayout* pSetLayouts, u32 pushConstantRangeCount,
    const VkPushConstantRange* pPushConstantRanges, VkPipelineLayoutCreateFlags flags, const void* next) {
    VkPipelineLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.pNext = next;
    createInfo.flags = flags;
    createInfo.pushConstantRangeCount = pushConstantRangeCount;
    createInfo.pPushConstantRanges = pPushConstantRanges;
    createInfo.setLayoutCount = setLayoutCount;
    createInfo.pSetLayouts = pSetLayouts;

    return create(dCtx, createInfo);
}

vulkanPipelineCreateInfoPack::vulkanPipelineCreateInfoPack(const vulkanPipelineCreateInfoPack& other) noexcept {
    _createInfo = other._createInfo;
    setCreateInfos();

    _vertexInputAttributes = other._vertexInputAttributes;
    _inputAssemblyStateCreateInfo = other._inputAssemblyStateCreateInfo;
    _tessellationStateCreateInfo = other._tessellationStateCreateInfo;
    _viewportStateCreateInfo = other._viewportStateCreateInfo;
    _rasterizationStateCreateInfo = other._rasterizationStateCreateInfo;
    _multisampleStateCreateInfo = other._multisampleStateCreateInfo;
    _depthStencilStateCreateInfo = other._depthStencilStateCreateInfo;
    _colorBlendStateCreateInfo = other._colorBlendStateCreateInfo;
    _dynamicStateCreateInfo = other._dynamicStateCreateInfo;

    _shaderStages = other._shaderStages;
    _vertexInputBindings = other._vertexInputBindings;
    _vertexInputAttributes = other._vertexInputAttributes;
    _viewports = other._viewports;
    _scissors = other._scissors;
    _colorBlendAttachmentStates = other._colorBlendAttachmentStates;
    _dynamicStates = other._dynamicStates;
    updateAllArrayAddresses();
}

void vulkanPipelineCreateInfoPack::updateAllArrays() {
    _createInfo.stageCount = _shaderStages.size();
    _vertexInputStateCreateInfo.vertexBindingDescriptionCount = _vertexInputBindings.size();
    _vertexInputStateCreateInfo.vertexAttributeDescriptionCount = _vertexInputAttributes.size();
    _viewportStateCreateInfo.scissorCount = _scissors.size();
    _viewportStateCreateInfo.viewportCount = _viewports.size();
    _colorBlendStateCreateInfo.attachmentCount = _colorBlendAttachmentStates.size();
    _dynamicStateCreateInfo.dynamicStateCount = _dynamicStates.size();
    updateAllArrayAddresses();
}

void vulkanPipelineCreateInfoPack::setCreateInfos() {
    _createInfo.pVertexInputState = &_vertexInputStateCreateInfo;
    _createInfo.pInputAssemblyState = &_inputAssemblyStateCreateInfo;
    _createInfo.pTessellationState = &_tessellationStateCreateInfo;
    _createInfo.pViewportState = &_viewportStateCreateInfo;
    _createInfo.pRasterizationState = &_rasterizationStateCreateInfo;
    _createInfo.pMultisampleState = &_multisampleStateCreateInfo;
    _createInfo.pDepthStencilState = &_depthStencilStateCreateInfo;
    _createInfo.pColorBlendState = &_colorBlendStateCreateInfo;
    _createInfo.pDynamicState = &_dynamicStateCreateInfo;
}

void vulkanPipelineCreateInfoPack::updateAllArrayAddresses() {
    _createInfo.pStages = _shaderStages.data();
    _vertexInputStateCreateInfo.pVertexBindingDescriptions = _vertexInputBindings.data();
    _vertexInputStateCreateInfo.pVertexAttributeDescriptions = _vertexInputAttributes.data();
    _viewportStateCreateInfo.pScissors = _scissors.data();
    _viewportStateCreateInfo.pViewports = _viewports.data();
    _colorBlendStateCreateInfo.pAttachments = _colorBlendAttachmentStates.data();
    _dynamicStateCreateInfo.pDynamicStates = _dynamicStates.data();
}

VkResult vulkanPipeline::create(const deviceContext& dCtx, const VkGraphicsPipelineCreateInfo& createInfo) {
    assert(_pipeline == VK_NULL_HANDLE && "::VULKAN:ERROR Can not recreate a pipeline\n");
    _device = dCtx._device;

    if (VkResult result = vkCreateGraphicsPipelines(_device, nullptr, 1, &createInfo, dCtx._allocator, &_pipeline)) {
        debug_error("::VULKAN:ERROR Failed to create a graphics pipeline\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanPipeline::create(const deviceContext& dCtx, const VkComputePipelineCreateInfo& createInfo) {
    assert(_pipeline == VK_NULL_HANDLE && "::VULKAN:ERROR Can not recreate a pipeline\n");
    _device = dCtx._device;

    if (VkResult result = vkCreateComputePipelines(_device, nullptr, 1, &createInfo, dCtx._allocator, &_pipeline)) {
        debug_error("::VULKAN:ERROR Failed to create a compute pipeline\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanPipeline::create(const deviceContext& dCtx, const VkRayTracingPipelineCreateInfoKHR& createInfo) {
    assert(_pipeline == VK_NULL_HANDLE && "::VULKAN:ERROR Can not recreate a pipeline\n");
    _device = dCtx._device;

    // TODO currently not useful because of deferredOperation
    if (VkResult result = vkCreateRayTracingPipelinesKHR(_device, nullptr, nullptr, 1, &createInfo, dCtx._allocator, &_pipeline)) {
        debug_error("::VULKAN:ERROR Failed to create a ray tracing pipeline\n");
        return result;
    }
    return VK_SUCCESS;
}
}
