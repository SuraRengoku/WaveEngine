#pragma once
#include "VulkanCommonHeaders.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

class vulkanRenderPass {
private:
    VkRenderPass            _renderPass{ VK_NULL_HANDLE };
    VkDevice                _device{ VK_NULL_HANDLE };

public:
    vulkanRenderPass() = default;
    vulkanRenderPass(VkDevice device) : _device(device) {}
    vulkanRenderPass(VkDevice device, const VkRenderPassCreateInfo& createInfo) : _device(device) {
        create(createInfo);
    }

    vulkanRenderPass(vulkanRenderPass&& other) noexcept {
        VK_MOVE_PTR(_renderPass);
        VK_MOVE_PTR(_device);
    }
    ~vulkanRenderPass() {
        VK_DESTROY_PTR_BY(vkDestroyRenderPass, _device, _renderPass);
    }

    VK_DEFINE_PTR_TYPE_OPERATOR(_renderPass);
    VK_DEFINE_ADDRESS_FUNCTION(_renderPass);

    const VkRenderPass& renderPass() const { return _renderPass; }

    void cmdBegin(VkCommandBuffer commandBuffer, VkRenderPassBeginInfo& beginInfo,
                VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE) const;
    void cmdBegin(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, VkRect2D renderArea,
                UTL::vector<const VkClearValue> clearValues = {}, VkSubpassContents = VK_SUBPASS_CONTENTS_INLINE) const;

    // next subpass
    void cmdNext(VkCommandBuffer commandBuffer, VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE) const {
        vkCmdNextSubpass(commandBuffer, subpassContents);
    }
    void cmdEnd(VkCommandBuffer commandBuffer) const {
        vkCmdEndRenderPass(commandBuffer);
    }

    VkResult create(const VkRenderPassCreateInfo& createInfo);
};

}