#include "VulkanSampler.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

VkResult vulkanSampler::create(const deviceContext& dCtx, const VkSamplerCreateInfo& createInfo) {
    assert(_sampler == VK_NULL_HANDLE && "::VULKAN:ERROR Can not recreate sampler\n");
    _device = dCtx._device;

    if (VkResult result = vkCreateSampler(_device, &createInfo, dCtx._allocator, &_sampler)) {
        debug_error("::VULKAN:ERROR Failed to create a sampler\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanSampler::create(const deviceContext& dCtx, const VkFilter& magFilter, const VkFilter& minFilter,
    const VkSamplerMipmapMode& mipmapMode, const VkSamplerAddressMode& addressModeU,
    const VkSamplerAddressMode& addressModeV, const VkSamplerAddressMode& addressModeW, float mipLodBias,
    VkBool32 anisotropyEnable, float maxAnisotropy, VkBool32 compareEnable, const VkCompareOp& compareOp, float minLod,
    float maxLod, const VkBorderColor& borderColor, VkBool32 unnormalizedCoordinates, VkSamplerCreateFlags flags,
    const void* next) {
    VkSamplerCreateInfo createInfo{};

    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.pNext = next;
    createInfo.flags = flags;
    createInfo.magFilter = magFilter;
    createInfo.minFilter = minFilter;
    createInfo.mipmapMode = mipmapMode;
    createInfo.addressModeU = addressModeU;
    createInfo.addressModeV = addressModeV;
    createInfo.addressModeW = addressModeW;
    createInfo.mipLodBias = mipLodBias;
    createInfo.anisotropyEnable = anisotropyEnable;
    createInfo.maxAnisotropy = maxAnisotropy;
    createInfo.compareEnable = compareEnable;
    createInfo.compareOp = compareOp;
    createInfo.minLod = minLod;
    createInfo.maxLod  = maxLod;
    createInfo.borderColor = borderColor;
    createInfo.unnormalizedCoordinates = unnormalizedCoordinates;

    return create(dCtx, createInfo);
}

}
