#include "VulkanImage.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

void vulkanImage::setDevice(VkDevice device) {
    assert(_device == VK_NULL_HANDLE);
    _device = device;
}

VkMemoryAllocateInfo vulkanImage::memoryAllocateInfo(VkPhysicalDevice physical_device, VkMemoryPropertyFlags desiredMemoryProperties) const {
    VkMemoryAllocateInfo memoryAllocateInfo{};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(_device, _image, &memoryRequirements);
    memoryAllocateInfo.allocationSize = memoryRequirements.size;

    auto getMemoryTypeIndex = [&](u32 memoryTypeBits, VkMemoryPropertyFlags desiredMemoryProperties) -> u32 {
        const auto& physicalDeviceMemoryProperties = VKX::findPhysicalDeviceMemoryProperties(physical_device);
        for (u32 i = 0; i < physicalDeviceMemoryProperties.memoryProperties.memoryTypeCount; ++i) {
            if (memoryTypeBits & 1 << i && (physicalDeviceMemoryProperties.memoryProperties.memoryTypes[i].propertyFlags & desiredMemoryProperties) == desiredMemoryProperties) {
                return i;
            }
        }
        return UINT32_MAX;
    };

    memoryAllocateInfo.memoryTypeIndex = getMemoryTypeIndex(memoryRequirements.memoryTypeBits, desiredMemoryProperties);
    if (memoryAllocateInfo.memoryTypeIndex == UINT32_MAX && desiredMemoryProperties & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) {
        memoryAllocateInfo.memoryTypeIndex = getMemoryTypeIndex(memoryRequirements.memoryTypeBits, desiredMemoryProperties & ~VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
    }
    // since we return memoryAllocateInfo out of this function, remember to check it when call this function
    return memoryAllocateInfo;
}

VkResult vulkanImage::bindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset) const {
    assert(_device != VK_NULL_HANDLE);
    if (VkResult result = vkBindImageMemory(_device, _image, deviceMemory, memoryOffset)) {
        debug_error("::VULKAN:ERROR Failed to attach image to memory\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanImage::create(VkImageCreateInfo& createInfo) {
    assert(_device != VK_NULL_HANDLE);
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    if (VkResult result = vkCreateImage(_device, &createInfo, nullptr, &_image)) {
        debug_error("::VULKAN:ERROR Failed to create an image\n");
        return result;
    }
    return VK_SUCCESS;
}

void vulkanImageView::setDevice(VkDevice device) {
    assert(_device == VK_NULL_HANDLE);
    _device = device;
}

VkResult vulkanImageView::create(const VkImageViewCreateInfo& createInfo) {
    assert(_device != VK_NULL_HANDLE);
    if (VkResult result = vkCreateImageView(_device, &createInfo, nullptr, &_image_view)) {
        debug_error("::VULKAN:ERROR Failed to create an image view\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanImageView::create(VkImage image, VkImageViewType viewType, VkFormat format,
    const VkImageSubresourceRange& subresourceRange, VkImageViewCreateFlags flags) {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image;
    createInfo.format = format;
    createInfo.subresourceRange = subresourceRange;
    createInfo.flags = flags;
    createInfo.viewType = viewType;
    return create(createInfo);
}

}
