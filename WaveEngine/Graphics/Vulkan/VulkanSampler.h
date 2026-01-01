#pragma once
#include "VulkanCommonHeaders.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

class vulkanSampler {
private:
    VkSampler                   _sampler{ VK_NULL_HANDLE };
    VkDevice                    _device{ VK_NULL_HANDLE };

public:
    vulkanSampler() = default;
    vulkanSampler(VkDevice device) : _device(device) {}
    vulkanSampler(VkDevice device, const VkSamplerCreateInfo& createInfo) : _device(device) {
        create(createInfo);
    }
    vulkanSampler(vulkanSampler&& other) noexcept {
        VK_MOVE_PTR(_sampler);
        VK_MOVE_PTR(_device);
    }
    ~vulkanSampler() {
        VK_DESTROY_PTR_BY(vkDestroySampler, _device, _sampler);
    }

    VK_DEFINE_PTR_TYPE_OPERATOR(_sampler);
    VK_DEFINE_ADDRESS_FUNCTION(_sampler);

    VkResult create(const VkSamplerCreateInfo& createInfo) {
        if (VkResult result = vkCreateSampler(_device, &createInfo, nullptr, &_sampler)) {
            debug_output("::VULKAN:ERROR Failed to create a sampler\n");
            return result;
        }
        return VK_SUCCESS;
    }
};

}