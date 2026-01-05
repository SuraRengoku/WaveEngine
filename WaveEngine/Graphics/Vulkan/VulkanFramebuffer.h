#pragma once
#include "VulkanCommonHeaders.h"
#include "VulkanContext.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

class vulkanFramebuffer {
private:
    VkFramebuffer                   _framebuffer{ VK_NULL_HANDLE };
    VkDevice                        _device { VK_NULL_HANDLE };

public:
    vulkanFramebuffer() = default;

    DISABLE_COPY(vulkanFramebuffer);

    vulkanFramebuffer(const deviceContext& dCtx, const VkFramebufferCreateInfo& createInfo) {
        create(dCtx, createInfo);
    }
    vulkanFramebuffer(const deviceContext& dCtx, VkRenderPass renderPass,
                    u32 attachmentCount, const VkImageView* pAttachments,
                    u32 width, u32 height, u32 layers,
                    const VkFramebufferCreateFlags& flags = 0, const void* next = nullptr) {
        create(dCtx, renderPass, attachmentCount, pAttachments, width, height, layers, flags, next);
    }

    VK_MOVE_CTOR2(vulkanFramebuffer, _framebuffer, _device);
    VK_MOVE_ASSIGN2(vulkanFramebuffer, _framebuffer, _device);

    ~vulkanFramebuffer() {
        VK_DESTROY_PTR_BY(vkDestroyFramebuffer, _device, _framebuffer);
    }

    [[nodiscard]] VK_DEFINE_PTR_TYPE_OPERATOR(_framebuffer);
    [[nodiscard]] VK_DEFINE_ADDRESS_FUNCTION(_framebuffer);

    [[nodiscard]] VkFramebuffer getFramebuffer() const noexcept {
        assert(_framebuffer != VK_NULL_HANDLE);
        return _framebuffer;
    }

    VkResult create(const deviceContext& dCtx, const VkFramebufferCreateInfo& createInfo);
    VkResult create(const deviceContext& dCtx, VkRenderPass renderPass,
                    u32 attachmentCount, const VkImageView* pAttachments,
                    u32 width, u32 height, u32 layers,
                    const VkFramebufferCreateFlags& flags = 0, const void* next = nullptr);

    bool isValid() const noexcept { return _framebuffer != VK_NULL_HANDLE; }
};

}
