#pragma once
#include "VulkanCommonHeaders.h"

namespace  WAVEENGINE::GRAPHICS::VULKAN {

class vulkanPipelineLayout {
private:
    VkPipelineLayout                _pipelineLayout{ VK_NULL_HANDLE };
    VkDevice                        _device{ VK_NULL_HANDLE };
public:
    vulkanPipelineLayout() = default;
    vulkanPipelineLayout(VkDevice device) : _device(device) {}
    vulkanPipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo&& createInfo) : _device(device) {
        create(createInfo);
    }
    vulkanPipelineLayout(vulkanPipelineLayout&& other) noexcept {
        VK_MOVE_PTR(_device);
        VK_MOVE_PTR(_pipelineLayout);
    }

    ~vulkanPipelineLayout() {
        VK_DESTROY_PTR_BY(vkDestroyPipelineLayout, _device, _pipelineLayout);
    }

    VK_DEFINE_PTR_TYPE_OPERATOR(_pipelineLayout);
    VK_DEFINE_ADDRESS_FUNCTION(_pipelineLayout);

    VkResult create(const VkPipelineLayoutCreateInfo& createInfo);
};

class vulkanPipelineCreateInfoPack {
    // Pipeline
    VkGraphicsPipelineCreateInfo _createInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    UTL::vector<VkPipelineShaderStageCreateInfo> _shaderStages;

    // Vertex Input
    VkPipelineVertexInputStateCreateInfo _vertexInputStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    UTL::vector<VkVertexInputBindingDescription>    _vertexInputBindings;
    UTL::vector<VkVertexInputAttributeDescription>  _vertexInputAttributes;

    // Input Assembly
    VkPipelineInputAssemblyStateCreateInfo _inputAssemblyStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };

    // Tessellation
    VkPipelineTessellationStateCreateInfo _tessellationStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };

    // Viewport
    VkPipelineViewportStateCreateInfo _viewportStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    UTL::vector<VkViewport> _viewports;
    UTL::vector<VkRect2D>   _scissors;
    u32                     _dynamicViewportCount = 1;
    u32                     _dynamicScissorCount = 1;

    // Rasterization
    VkPipelineRasterizationStateCreateInfo _rasterizationStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };

    // Multisample
    VkPipelineMultisampleStateCreateInfo _multisampleStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };

    // Depth & Stencil
    VkPipelineDepthStencilStateCreateInfo _depthStencilStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

    // Color Blending
    VkPipelineColorBlendStateCreateInfo _colorBlendStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    UTL::vector<VkPipelineColorBlendAttachmentState> _colorBlendAttachmentStates;

    // Dynamic
    VkPipelineDynamicStateCreateInfo _dynamicStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    UTL::vector<VkDynamicState> _dynamicStates;

    vulkanPipelineCreateInfoPack() {
        setCreateInfos();
        _createInfo.basePipelineIndex = -1; // not a derived pipeline
    }

    vulkanPipelineCreateInfoPack(const vulkanPipelineCreateInfoPack& other) noexcept;

    operator VkGraphicsPipelineCreateInfo& () { return _createInfo; }

    void updateAllArrays();
private:
    void setCreateInfos();
    void updateAllArrayAddresses();
};

class vulkanPipeline {
private:
    VkPipeline                  _pipeline{ VK_NULL_HANDLE };
    VkDevice                    _device{ VK_NULL_HANDLE };

public:
    vulkanPipeline() = default;
    vulkanPipeline(VkDevice device) : _device(device) {}
    vulkanPipeline(VkDevice device, const VkGraphicsPipelineCreateInfo& createInfo) : _device(device) {
        create(createInfo);
    }
    vulkanPipeline(VkDevice device, const VkComputePipelineCreateInfo& createInfo) : _device(device) {
        create(createInfo);
    }
    vulkanPipeline(VkDevice device, const VkRayTracingPipelineCreateInfoKHR& createInfo) : _device(device) {
        create(createInfo);
    }

    vulkanPipeline(vulkanPipeline&& other) noexcept {
        VK_MOVE_PTR(_pipeline);
        VK_MOVE_PTR(_device);
    }
    ~vulkanPipeline() {
        VK_DESTROY_PTR_BY(vkDestroyPipeline, _device, _pipeline);
    }

    VK_DEFINE_PTR_TYPE_OPERATOR(_pipeline);
    VK_DEFINE_ADDRESS_FUNCTION(_pipeline);

    void setDevice(VkDevice device) {
        _device = device;
    }

    VkResult create(const VkGraphicsPipelineCreateInfo& createInfo);
    VkResult create(const VkComputePipelineCreateInfo& createInfo);

    // TODO currently not useful because of deferred operation
    VkResult create(const VkRayTracingPipelineCreateInfoKHR& createInfo);
};

}
