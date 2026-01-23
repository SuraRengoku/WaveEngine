#pragma once
#include "VulkanCommonHeaders.h"
#include "VulkanContext.h"

namespace  WAVEENGINE::GRAPHICS::VULKAN {

struct pipelineLayoutImpl {
    VkDevice            device{ VK_NULL_HANDLE };
    VkPipelineLayout    layout{ VK_NULL_HANDLE };

    ~pipelineLayoutImpl() {
        if (layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, layout, nullptr);
        }
    }
};

class vulkanPipelineLayout {
public:
    vulkanPipelineLayout() = default;

    // non-owning view(optional)
    explicit vulkanPipelineLayout(VkPipelineLayout layout) {
        auto impl = std::make_shared<pipelineLayoutImpl>();
        impl->layout = layout;
        _impl = impl;
    }

    vulkanPipelineLayout(const deviceContext& dCtx, const VkPipelineLayoutCreateInfo& createInfo) {
        create(dCtx, createInfo);
    }
    vulkanPipelineLayout(const deviceContext& dCtx,
                         u32 setLayoutCount, const VkDescriptorSetLayout* pSetLayouts,
                         u32 pushConstantRangeCount, const VkPushConstantRange* pPushConstantRanges,
                         VkPipelineLayoutCreateFlags flags = 0, const void* next = nullptr) {
        create(dCtx, setLayoutCount, pSetLayouts, pushConstantRangeCount, pPushConstantRanges, flags, next);
    }

    [[nodiscard]] VkPipelineLayout handle() const {
        return _impl ? _impl->layout : VK_NULL_HANDLE;
    }

    [[nodiscard]] bool isValid() const noexcept {
        return _impl && _impl->layout != VK_NULL_HANDLE;
    }

    VkResult create(const deviceContext& dCtx, const VkPipelineLayoutCreateInfo& createInfo);
    VkResult create(const deviceContext& dCtx,
                    u32 setLayoutCount, const VkDescriptorSetLayout* pSetLayouts,
                    u32 pushConstantRangeCount, const VkPushConstantRange* pPushConstantRanges,
                    VkPipelineLayoutCreateFlags flags = 0, const void* next = nullptr);

private:
    std::shared_ptr<pipelineLayoutImpl> _impl;
};


struct vulkanGraphicsPipelineCreateInfoPack {
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
    u8                      _viewportCount = 1;
    u8                      _scissorCount = 1;

    // Rasterization
    VkPipelineRasterizationStateCreateInfo _rasterizationStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };

    // Multisample
    VkPipelineMultisampleStateCreateInfo _multisampleStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };

    // Depth & Stencil
    VkPipelineDepthStencilStateCreateInfo _depthStencilStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

    // Color Blending
    VkPipelineColorBlendStateCreateInfo _colorBlendStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    UTL::vector<VkPipelineColorBlendAttachmentState> _colorBlendAttachmentStates;

    // ========================================= Dynamic Rendering ========================================
    
#define VK_DEFINE_DYNAMIC_STATE_TOGGLE(name, state) \
    void enableDynamic##name(bool enable = true) {  \
		if (enable) addDynamicState(state);         \
        else removeDynamicState(state);             \
	}

	VkPipelineDynamicStateCreateInfo _dynamicStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    UTL::vector<VkDynamicState> _dynamicStates;

    void setDynamicViewport(u32 count);
    void setStaticViewport();
    void setDynamicScissor(u32 count);
    void setStaticScissor();
    void setDynamicViewportScissor(u32 count);
    void setStaticViewportScissor();

    void enableDynamicStencil(bool enable = true);

    // -------------------------------------- Base Dynamic Feature ---------------------------------------

    VK_DEFINE_DYNAMIC_STATE_TOGGLE(DepthBias, VK_DYNAMIC_STATE_DEPTH_BIAS)
    VK_DEFINE_DYNAMIC_STATE_TOGGLE(DepthBounds, VK_DYNAMIC_STATE_DEPTH_BOUNDS)
    VK_DEFINE_DYNAMIC_STATE_TOGGLE(BlendConstants, VK_DYNAMIC_STATE_BLEND_CONSTANTS)

    // ----------------------------- VK 1.3 / VK_EXT_extended_dynamic_state ------------------------------

    void setDynamicViewportWithCount();
    void setDynamicScissorWithCount();
    
	VK_DEFINE_DYNAMIC_STATE_TOGGLE(CullMode, VK_DYNAMIC_STATE_CULL_MODE)
    VK_DEFINE_DYNAMIC_STATE_TOGGLE(FrontFace, VK_DYNAMIC_STATE_FRONT_FACE)
    VK_DEFINE_DYNAMIC_STATE_TOGGLE(PrimitiveTopology, VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY)

    VK_DEFINE_DYNAMIC_STATE_TOGGLE(DepthTestEnable, VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE)
    VK_DEFINE_DYNAMIC_STATE_TOGGLE(DepthWriteEnable, VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE)
    VK_DEFINE_DYNAMIC_STATE_TOGGLE(DepthCompareOp, VK_DYNAMIC_STATE_DEPTH_COMPARE_OP)

	// ----------------------------- VK 1.3 / VK_EXT_extended_dynamic_state2 -----------------------------
    
    VK_DEFINE_DYNAMIC_STATE_TOGGLE(RasterizerDiscardEnable, VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE)
    VK_DEFINE_DYNAMIC_STATE_TOGGLE(PrimitiveRestartEnable, VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE)

	// ----------------------------- VK 1.3 / VK_EXT_extended_dynamic_state3 -----------------------------

    VK_DEFINE_DYNAMIC_STATE_TOGGLE(PolygonMode, VK_DYNAMIC_STATE_POLYGON_MODE_EXT)
    VK_DEFINE_DYNAMIC_STATE_TOGGLE(ColorBlendEnable, VK_DYNAMIC_STATE_COLOR_BLEND_ENABLE_EXT)

    // ====================================================================================================

    vulkanGraphicsPipelineCreateInfoPack() {
        setCreateInfos();
        _createInfo.basePipelineIndex = -1; // not a derived pipeline
    }

    vulkanGraphicsPipelineCreateInfoPack(const vulkanGraphicsPipelineCreateInfoPack& other) noexcept;

    operator VkGraphicsPipelineCreateInfo& () { return _createInfo; }

    void updateAllArrays();
private:
    void setCreateInfos();
    void updateAllArrayAddresses();

    void addDynamicState(VkDynamicState state);
    void removeDynamicState(VkDynamicState state);

#undef VK_DEFINE_DYNAMIC_STATE_TOGGLE
};


class vulkanPipeline {
private:
    VkPipeline                  _pipeline{ VK_NULL_HANDLE };
    VkDevice                    _device{ VK_NULL_HANDLE };

public:
    vulkanPipeline() = default;
    vulkanPipeline(const deviceContext& dCtx, const VkGraphicsPipelineCreateInfo& createInfo)  {
        create(dCtx, createInfo);
    }
    vulkanPipeline(const deviceContext& dCtx, const VkComputePipelineCreateInfo& createInfo) {
        create(dCtx, createInfo);
    }
    vulkanPipeline(const deviceContext& dCtx,  const VkRayTracingPipelineCreateInfoKHR& createInfo) {
        create(dCtx, createInfo);
    }

    VK_MOVE_CTOR2(vulkanPipeline, _pipeline, _device)
    VK_MOVE_ASSIGN2(vulkanPipeline, _pipeline, _device)

    ~vulkanPipeline() {
        destroy();
    }

    [[nodiscard]] VK_DEFINE_PTR_TYPE_OPERATOR(_pipeline)
    [[nodiscard]] VK_DEFINE_ADDRESS_FUNCTION(_pipeline)

    [[nodiscard]] VkPipeline pipeline() const {
        assert(_pipeline != VK_NULL_HANDLE);
        return _pipeline;
    }

    // we hope that the createInfo can be defined explicitly before create the pipeline.
    // Thus, we do not provide detailed-parameter create function.
    VkResult create(const deviceContext& dCtx, const VkGraphicsPipelineCreateInfo& createInfo);
    VkResult create(const deviceContext& dCtx, const VkComputePipelineCreateInfo& createInfo);

    // TODO currently not useful because of lacking deferred operation
    VkResult create(const deviceContext& dCtx, const VkRayTracingPipelineCreateInfoKHR& createInfo);

    void destroy() noexcept;

    bool isValid() const noexcept { return _pipeline; }
};

}
