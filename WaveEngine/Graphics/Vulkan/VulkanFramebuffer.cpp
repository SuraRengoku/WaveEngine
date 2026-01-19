#include "VulkanFramebuffer.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

VkResult vulkanFramebuffer::create(const deviceContext& dCtx, const VkFramebufferCreateInfo& createInfo) {
    assert(_framebuffer == VK_NULL_HANDLE && "::VULKAN:ERROR Can not recreate framebuffer\n");
    _device = dCtx._device;

    if (VkResult result = vkCreateFramebuffer(_device, &createInfo, dCtx._allocator, &_framebuffer)) {
        debug_error("::VULKAN:ERROR Failed to create a framebuffer\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanFramebuffer::create(const deviceContext& dCtx, VkRenderPass renderPass, u32 attachmentCount,
    const VkImageView* pAttachments, u32 width, u32 height, u32 layers, const VkFramebufferCreateFlags& flags,
    const void* next) {
    VkFramebufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.flags = flags;
    createInfo.pNext = next;
    createInfo.renderPass = renderPass;
    createInfo.attachmentCount = attachmentCount;
    createInfo.pAttachments = pAttachments;
    createInfo.width = width;
    createInfo.height = height;
    createInfo.layers = layers;

    return create(dCtx, createInfo);
}

void vulkanFramebuffer::destroy() noexcept {
	if (_framebuffer != VK_NULL_HANDLE) {
        VK_DESTROY_PTR_BY(vkDestroyFramebuffer, _device, _framebuffer)
        _device = VK_NULL_HANDLE;
	}
}
}
