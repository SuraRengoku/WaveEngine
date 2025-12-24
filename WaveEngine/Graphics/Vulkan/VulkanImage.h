#pragma once
#include "VulkanCommonHeaders.h"
#include "VulkanCore.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

class vulkanImage {
private:
    VkImage                        _image{ VK_NULL_HANDLE };

public:
    vulkanImage() = default;
    vulkanImage(VkImageCreateInfo& createInfo) {
        create(createInfo);
    }
    DISABLE_COPY(vulkanImage);
    vulkanImage(vulkanImage&& other) noexcept {
        VK_MOVE_PTR(_image);
    }
    ~vulkanImage() { VK_DESTROY_PTR_BY(vkDestroyImage, CORE::device(), _image); }

    VK_DEFINE_PTR_TYPE_OPERATOR(_image);
    VK_DEFINE_ADDRESS_FUNCTION(_image);

    VkMemoryAllocateInfo memoryAllocateInfo(VkMemoryPropertyFlags desiredMemoryProperties) const;

    VkResult bindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset = 0) const;

    VkResult create(VkImageCreateInfo& createInfo);
};

class vulkanImageView {
private:
    VkImageView                 _image_view{ VK_NULL_HANDLE };
public:
    vulkanImageView() = default;
    vulkanImageView(VkImageViewCreateInfo& creatInfo) {

    }
    DISABLE_COPY(vulkanImageView);
    vulkanImageView(vulkanImageView&& other) noexcept { VK_MOVE_PTR(_image_view); }
    ~vulkanImageView() { VK_DESTROY_PTR_BY(vkDestroyImageView, CORE::device(), _image_view); }

    VK_DEFINE_PTR_TYPE_OPERATOR(_image_view);
    VK_DEFINE_ADDRESS_FUNCTION(_image_view);

    VkResult create(VkImageViewCreateInfo& createInfo);

    VkResult create(VkImage image, VkImageViewType viewType, VkFormat format, const VkImageSubresourceRange& subresourceRange, VkImageViewCreateFlags flags = 0);
};

}

