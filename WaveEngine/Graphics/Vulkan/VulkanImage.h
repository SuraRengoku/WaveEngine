#pragma once
#include "VulkanCommonHeaders.h"
#include "VulkanCore.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

class vulkanImage {
private:
    VkImage                        _image{ VK_NULL_HANDLE };
    VkDevice                       _device{ VK_NULL_HANDLE };

public:
    vulkanImage() = default;
    vulkanImage(VkDevice device) : _device(device) {}
    vulkanImage(VkDevice device, VkImageCreateInfo& createInfo) : _device(device) {
        create(createInfo);
    }
    DISABLE_COPY(vulkanImage);
    vulkanImage(vulkanImage&& other) noexcept {
        VK_MOVE_PTR(_image);
        VK_MOVE_PTR(_device);
    }
    ~vulkanImage() { VK_DESTROY_PTR_BY(vkDestroyImage, _device, _image); }

    VK_DEFINE_PTR_TYPE_OPERATOR(_image);
    VK_DEFINE_ADDRESS_FUNCTION(_image);

    // delayed injection
    void setDevice(VkDevice device);

    VkMemoryAllocateInfo memoryAllocateInfo(VkPhysicalDevice physical_device, VkMemoryPropertyFlags desiredMemoryProperties) const;

    VkResult bindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset = 0) const;

    VkResult create(VkImageCreateInfo& createInfo);
};

class vulkanImageView {
private:
    VkImageView                 _image_view{ VK_NULL_HANDLE };
    VkDevice                    _device{ VK_NULL_HANDLE };

public:
    vulkanImageView() = default;
    vulkanImageView(VkDevice device) : _device(device) {}
    vulkanImageView(VkDevice device, const VkImageViewCreateInfo& createInfo) : _device(device) {
        create(createInfo);
    }
    DISABLE_COPY(vulkanImageView);
    vulkanImageView(vulkanImageView&& other) noexcept {
        VK_MOVE_PTR(_image_view);
        VK_MOVE_PTR(_device);
    }
    ~vulkanImageView() { VK_DESTROY_PTR_BY(vkDestroyImageView, _device, _image_view); }

    VK_DEFINE_PTR_TYPE_OPERATOR(_image_view);
    VK_DEFINE_ADDRESS_FUNCTION(_image_view);

    // delayed injection
    void setDevice(VkDevice device);

    VkResult create(const VkImageViewCreateInfo& createInfo);

    VkResult create(VkImage image, VkImageViewType viewType, VkFormat format, const VkImageSubresourceRange& subresourceRange, VkImageViewCreateFlags flags = 0);
};

}

