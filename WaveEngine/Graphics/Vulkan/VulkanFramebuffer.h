#pragma once
#include "VulkanCommonHeaders.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

class vulkanFramebuffer {
private:
    VkFramebuffer                   _framebuffer{ VK_NULL_HANDLE };
    VkDevice                        _device { VK_NULL_HANDLE };

public:
    vulkanFramebuffer() = default;
    vulkanFramebuffer(VkDevice device) : _device(device) {}
    vulkanFramebuffer(VkDevice device, const VkFramebufferCreateInfo& createInfo) : _device(device) {
        create(createInfo);
    }
    vulkanFramebuffer(vulkanFramebuffer&& other) noexcept {
        VK_MOVE_PTR(_framebuffer);
        VK_MOVE_PTR(_device);
    }
    ~vulkanFramebuffer() {
        VK_DESTROY_PTR_BY(vkDestroyFramebuffer, _device, _framebuffer);
    }

    VK_DEFINE_PTR_TYPE_OPERATOR(_framebuffer);
    VK_DEFINE_ADDRESS_FUNCTION(_framebuffer);

    VkResult create(const VkFramebufferCreateInfo& createInfo);
};

}
