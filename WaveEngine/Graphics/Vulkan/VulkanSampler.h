#pragma once
#include "VulkanCommonHeaders.h"
#include "VulkanContext.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

class vulkanSampler {
private:
    VkSampler                   _sampler{ VK_NULL_HANDLE };
    VkDevice                    _device{ VK_NULL_HANDLE };

public:
    vulkanSampler() = default;
    vulkanSampler(const deviceContext& dCtx, const VkSamplerCreateInfo& createInfo) {
        create(dCtx, createInfo);
    }
    vulkanSampler(const deviceContext& dCtx, const VkFilter& magFilter, const VkFilter& minFilter,
                    const VkSamplerMipmapMode& mipmapMode, const VkSamplerAddressMode& addressModeU,
                    const VkSamplerAddressMode& addressModeV, const VkSamplerAddressMode& addressModeW,
                    float mipLodBias, VkBool32 anisotropyEnable, float maxAnisotropy,
                    VkBool32 compareEnable, const VkCompareOp& compareOp, float minLod, float maxLod,
                    const VkBorderColor& borderColor, VkBool32 unnormalizedCoordinates,
                    VkSamplerCreateFlags flags = 0, const void* next = nullptr) {
        create(dCtx, magFilter, minFilter, mipmapMode, addressModeU, addressModeV, addressModeW,
            mipLodBias, anisotropyEnable, maxAnisotropy, compareEnable, compareOp, minLod, maxLod,
            borderColor, unnormalizedCoordinates, flags, next);
    }

    VK_MOVE_CTOR2(vulkanSampler, _sampler, _device)
    VK_MOVE_ASSIGN2(vulkanSampler, _sampler, _device)

    ~vulkanSampler() {
        VK_DESTROY_PTR_BY(vkDestroySampler, _device, _sampler)
    }

    [[nodiscard]] VK_DEFINE_PTR_TYPE_OPERATOR(_sampler)
    [[nodiscard]] VK_DEFINE_ADDRESS_FUNCTION(_sampler)

    [[nodiscard]] VkSampler sampler() const {
        assert(_sampler != VK_NULL_HANDLE);
        return _sampler;
    }

    VkResult create(const deviceContext& dCtx, const VkSamplerCreateInfo& createInfo);
    VkResult create(const deviceContext& dCtx, const VkFilter& magFilter, const VkFilter& minFilter,
                    const VkSamplerMipmapMode& mipmapMode, const VkSamplerAddressMode& addressModeU,
                    const VkSamplerAddressMode& addressModeV, const VkSamplerAddressMode& addressModeW,
                    float mipLodBias, VkBool32 anisotropyEnable, float maxAnisotropy,
                    VkBool32 compareEnable, const VkCompareOp& compareOp, float minLod, float maxLod,
                    const VkBorderColor& borderColor, VkBool32 unnormalizedCoordinates,
                    VkSamplerCreateFlags flags = 0, const void* next = nullptr);

    bool isValid() const noexcept { return _sampler != VK_NULL_HANDLE; }
};

}