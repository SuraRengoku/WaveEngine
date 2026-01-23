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

void vulkanGraphicsPipelineCreateInfoPack::setDynamicViewport(u32 count) {
    assert(count > 0 && "Viewport count must be > 0 for VK_DYNAMIC_STATE_VIEWPORT "
        "Use setDynamicViewportWithCount() if you need variable count.");

    removeDynamicState(VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT);
    addDynamicState(VK_DYNAMIC_STATE_VIEWPORT);

    _viewportStateCreateInfo.viewportCount = _viewportCount = count;
    _viewportStateCreateInfo.pViewports = nullptr;
}

void vulkanGraphicsPipelineCreateInfoPack::setStaticViewport() {
    removeDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
    removeDynamicState(VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT);

    _viewportStateCreateInfo.viewportCount = _viewportCount = static_cast<u32>(_viewports.size());
    _viewportStateCreateInfo.pViewports = _viewports.data();

    assert(!_viewports.empty() && "Must add viewports before using static viewport state");
}

void vulkanGraphicsPipelineCreateInfoPack::setDynamicScissor(u32 count) {
    assert(count > 0 && "Scissor count must be > 0 for VK_DYNAMIC_STATE_SCISSOR "
        "Use setDynamicScissorWithCount() if you need variable count.");

    removeDynamicState(VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT);
    addDynamicState(VK_DYNAMIC_STATE_SCISSOR);

    _viewportStateCreateInfo.scissorCount = _scissorCount = count;
    _viewportStateCreateInfo.pScissors = nullptr;
}

void vulkanGraphicsPipelineCreateInfoPack::setStaticScissor() {
    removeDynamicState(VK_DYNAMIC_STATE_SCISSOR);
    removeDynamicState(VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT);

    _viewportStateCreateInfo.scissorCount = _scissorCount = static_cast<u32>(_scissors.size());
    _viewportStateCreateInfo.pScissors = _scissors.data();

    assert(!_scissors.empty() && "Must add scissors before using static scissor state");
}

void vulkanGraphicsPipelineCreateInfoPack::setDynamicViewportScissor(u32 count) {
    setDynamicViewport(count);
    setDynamicScissor(count);
}

void vulkanGraphicsPipelineCreateInfoPack::setStaticViewportScissor() {
    setStaticViewport();
    setStaticScissor();
}

void vulkanGraphicsPipelineCreateInfoPack::setDynamicViewportWithCount() {
    removeDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
    addDynamicState(VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT);

    _viewportStateCreateInfo.viewportCount = _viewportCount = 0;
    _viewportStateCreateInfo.pViewports = nullptr;
}

void vulkanGraphicsPipelineCreateInfoPack::setDynamicScissorWithCount() {
    removeDynamicState(VK_DYNAMIC_STATE_SCISSOR);
    addDynamicState(VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT);

    _viewportStateCreateInfo.scissorCount = _scissorCount = 0;
    _viewportStateCreateInfo.pScissors = nullptr;
}

void vulkanGraphicsPipelineCreateInfoPack::enableDynamicStencil(bool enable) {
	if (enable) {
        addDynamicState(VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK);
        addDynamicState(VK_DYNAMIC_STATE_STENCIL_WRITE_MASK);
        addDynamicState(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
	} else {
        removeDynamicState(VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK);
        removeDynamicState(VK_DYNAMIC_STATE_STENCIL_WRITE_MASK);
        removeDynamicState(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
	}
}

vulkanGraphicsPipelineCreateInfoPack::vulkanGraphicsPipelineCreateInfoPack(const vulkanGraphicsPipelineCreateInfoPack& other) noexcept {
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
    _viewportCount = other._viewportCount;
    _scissors = other._scissors;
    _scissorCount = other._scissorCount;
    _colorBlendAttachmentStates = other._colorBlendAttachmentStates;
    _dynamicStates = other._dynamicStates;

    updateAllArrayAddresses();
}

void vulkanGraphicsPipelineCreateInfoPack::updateAllArrays() {
    _createInfo.stageCount = static_cast<u32>(_shaderStages.size());
    _vertexInputStateCreateInfo.vertexBindingDescriptionCount = static_cast<u32>(_vertexInputBindings.size());
    _vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<u32>(_vertexInputAttributes.size());
    _viewportStateCreateInfo.scissorCount = _scissorCount;
    _viewportStateCreateInfo.viewportCount = _viewportCount;
    _colorBlendStateCreateInfo.attachmentCount = static_cast<u32>(_colorBlendAttachmentStates.size());
    _dynamicStateCreateInfo.dynamicStateCount = static_cast<u32>(_dynamicStates.size());
    updateAllArrayAddresses();
}

void vulkanGraphicsPipelineCreateInfoPack::setCreateInfos() {
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

void vulkanGraphicsPipelineCreateInfoPack::updateAllArrayAddresses() {
    _createInfo.pStages = _shaderStages.empty() ? nullptr : _shaderStages.data();
    _vertexInputStateCreateInfo.pVertexBindingDescriptions = _vertexInputBindings.empty() ? nullptr : _vertexInputBindings.data();
    _vertexInputStateCreateInfo.pVertexAttributeDescriptions = _vertexInputAttributes.empty() ? nullptr : _vertexInputAttributes.data();
   
    // For dynamic state, these should be nullptr and are set in setDynamic* methods
    // For static state, these are set in setStatic* methods
	_viewportStateCreateInfo.pScissors = _scissors.empty() ? nullptr : _scissors.data();
    _viewportStateCreateInfo.pViewports = _viewports.empty() ? nullptr : _viewports.data();

    _colorBlendStateCreateInfo.pAttachments = _colorBlendAttachmentStates.empty() ? nullptr : _colorBlendAttachmentStates.data();
    _dynamicStateCreateInfo.pDynamicStates = _dynamicStates.empty() ? nullptr : _dynamicStates.data();
}

void vulkanGraphicsPipelineCreateInfoPack::addDynamicState(VkDynamicState state) {
    auto it = std::find(_dynamicStates.begin(), _dynamicStates.end(), state);
    if (it == _dynamicStates.end()) {
        _dynamicStates.push_back(state);
    }
}

void vulkanGraphicsPipelineCreateInfoPack::removeDynamicState(VkDynamicState state) {
    auto it = std::find(_dynamicStates.begin(), _dynamicStates.end(), state);
    if (it != _dynamicStates.end()) {
        _dynamicStates.erase(it);
    }
}

VkResult vulkanPipeline::create(const deviceContext& dCtx, const VkGraphicsPipelineCreateInfo& createInfo) {
    assert(_pipeline == VK_NULL_HANDLE && "::VULKAN:ERROR Can not recreate a graphics pipeline\n");
    _device = dCtx._device;

    if (VkResult result = vkCreateGraphicsPipelines(_device, nullptr, 1, &createInfo, dCtx._allocator, &_pipeline)) {
        debug_error("::VULKAN:ERROR Failed to create a graphics pipeline\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanPipeline::create(const deviceContext& dCtx, const VkComputePipelineCreateInfo& createInfo) {
    assert(_pipeline == VK_NULL_HANDLE && "::VULKAN:ERROR Can not recreate a compute pipeline\n");
    _device = dCtx._device;

    if (VkResult result = vkCreateComputePipelines(_device, nullptr, 1, &createInfo, dCtx._allocator, &_pipeline)) {
        debug_error("::VULKAN:ERROR Failed to create a compute pipeline\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanPipeline::create(const deviceContext& dCtx, const VkRayTracingPipelineCreateInfoKHR& createInfo) {
    assert(_pipeline == VK_NULL_HANDLE && "::VULKAN:ERROR Can not recreate a ray tracing pipeline\n");
    _device = dCtx._device;

    if (vulkanContext::vkCreateRayTracingPipelinesKHR == nullptr) {
        debug_error("::VULKAN:ERROR Ray tracing not supported - vkCreateRayTracingPipelinesKHR not available\n");
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    // TODO currently not useful because of deferred Operation
    if (VkResult result = vulkanContext::vkCreateRayTracingPipelinesKHR(
        _device, 
        VK_NULL_HANDLE, 
        VK_NULL_HANDLE, 
        1, 
        &createInfo, 
        dCtx._allocator, 
        &_pipeline)) {
        debug_error("::VULKAN:ERROR Failed to create a ray tracing pipeline\n");
        return result;
    }

#ifdef _DEBUG
    debug_output("::VULKAN:INFO Ray tracing pipeline created\n");
#endif

    return VK_SUCCESS;
}

void vulkanPipeline::destroy() noexcept {
	if (_pipeline != VK_NULL_HANDLE) {
        VK_DESTROY_PTR_BY(vkDestroyPipeline, _device, _pipeline)
        _device = VK_NULL_HANDLE;
	}
}
}
