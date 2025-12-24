#include "VulkanImage.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

VkMemoryAllocateInfo vulkanImage::memoryAllocateInfo(VkMemoryPropertyFlags desiredMemoryProperties) const {
    VkMemoryAllocateInfo memoryAllocateInfo{};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(CORE::device(), _image, &memoryRequirements);
    memoryAllocateInfo.allocationSize = memoryRequirements.size;

    auto getMemoryTypeIndex = [](u32 memoryTypeBits, VkMemoryPropertyFlags desiredMemoryProperties) {
        const auto& physicalDeviceMemoryProperties = VKX::findPhysicalDeviceMemoryProperties(CORE::physical_device());
        for (u32 i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i) {
            if (memoryTypeBits & 1 << i && (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & desiredMemoryProperties) == desiredMemoryProperties) {
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
    if (VkResult result = vkBindImageMemory(CORE::device(), _image, deviceMemory, memoryOffset)) {
        debug_output("::VULKAN: Failed to attach image to memory\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanImage::create(VkImageCreateInfo& createInfo) {
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    if (VkResult result = vkCreateImage(CORE::device(), &createInfo, nullptr, &_image)) {
        debug_output("::VULKAN: Failed to create an image\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanImageView::create(VkImageViewCreateInfo& createInfo) {
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    if (VkResult result = vkCreateImageView(CORE::device(), &createInfo, nullptr, &_image_view)) {
        debug_output("::VULKAN: Failed to create an image view\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanImageView::create(VkImage image, VkImageViewType viewType, VkFormat format,
    const VkImageSubresourceRange& subresourceRange, VkImageViewCreateFlags flags) {
    VkImageViewCreateInfo createInfo{};
    createInfo.image = image;
    createInfo.format = format;
    createInfo.subresourceRange = subresourceRange;
    createInfo.flags = flags;
    createInfo.viewType = viewType;
    return create(createInfo);
}

}
