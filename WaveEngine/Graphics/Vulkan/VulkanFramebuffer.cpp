#include "VulkanFramebuffer.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

VkResult vulkanFramebuffer::create(const VkFramebufferCreateInfo& createInfo) {
    assert(_device != VK_NULL_HANDLE);
    if (VkResult result = vkCreateFramebuffer(_device, &createInfo, nullptr, &_framebuffer)) {
        debug_output("::VULKAN: failed to create a framebuffer");
        return result;
    }
    return VK_SUCCESS;
}
}
