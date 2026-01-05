#pragma once
#include "VulkanCommonHeaders.h"
#include "VulkanContext.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

class vulkanRenderPass {
private:
    VkRenderPass            _renderPass{ VK_NULL_HANDLE };
    VkDevice                _device{ VK_NULL_HANDLE };

public:
    vulkanRenderPass() = default;

    DISABLE_COPY(vulkanRenderPass);

    vulkanRenderPass(const deviceContext& dCtx, const VkRenderPassCreateInfo& createInfo) {
        create(dCtx, createInfo);
    }
    vulkanRenderPass(const deviceContext& dCtx,
                    u32 attachmentCount, const VkAttachmentDescription* pAttachments,
                    u32 subpassCount, const VkSubpassDescription* pSubpass,
                    u32 dependencyCount, const VkSubpassDependency* pDependencies,
                    const VkRenderPassCreateFlags& flags = 0, const void* next = nullptr) {
        create(dCtx, attachmentCount, pAttachments, subpassCount, pSubpass, dependencyCount, pDependencies, flags, next);
    }

    VK_MOVE_CTOR2(vulkanRenderPass, _renderPass, _device);
    VK_MOVE_ASSIGN2(vulkanRenderPass, _renderPass, _device);

    ~vulkanRenderPass() {
        VK_DESTROY_PTR_BY(vkDestroyRenderPass, _device, _renderPass);
    }

    [[nodiscard]] VK_DEFINE_PTR_TYPE_OPERATOR(_renderPass);
    [[nodiscard]] VK_DEFINE_ADDRESS_FUNCTION(_renderPass);

    [[nodiscard]] const VkRenderPass& renderPass() const {
        assert(_renderPass != VK_NULL_HANDLE);
        return _renderPass;
    }

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

    VkResult create(const deviceContext& dCtx, const VkRenderPassCreateInfo& createInfo);
    VkResult create(const deviceContext& dCtx,
                    u32 attachmentCount, const VkAttachmentDescription* pAttachments,
                    u32 subpassCount, const VkSubpassDescription* pSubpass,
                    u32 dependencyCount, const VkSubpassDependency* pDependencies,
                    const VkRenderPassCreateFlags& flags = 0, const void* next = nullptr);

    bool isValid() const noexcept { return _renderPass != VK_NULL_HANDLE; }
};

}