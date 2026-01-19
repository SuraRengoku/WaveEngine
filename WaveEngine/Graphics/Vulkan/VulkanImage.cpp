#include "VulkanImage.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {
bool vulkanImage::create(const deviceContext& dCtx, const VkImageCreateInfo& createInfo) {
    assert(_image == VK_NULL_HANDLE && "::VULKAN:ERROR Can not recreate an image\n");
    _device = dCtx._device;

    VKCall(vkCreateImage(_device, &createInfo, dCtx._allocator, &_image), "::VULKAN:ERROR Failed to create an image\n");
    return true;
}

bool vulkanImage::create(const deviceContext& dCtx, VkImageType type, VkFormat format,
                         VkExtent3D extent, u32 mLevels, u32 aLayers, VkSampleCountFlagBits samples,
                         VkImageTiling tiling, VkSharingMode sMode, VkImageLayout iniLayout,
                         const VkImageUsageFlags uFlags, const VkImageCreateFlags cFlags, const void* next) {
    VkImageCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.pNext = next;
    createInfo.flags = cFlags;
    createInfo.imageType = type;
    createInfo.format = format;
    createInfo.extent = extent;
    createInfo.sharingMode = sMode;
    createInfo.usage = uFlags;
    createInfo.arrayLayers = aLayers;
    createInfo.initialLayout = iniLayout;
    createInfo.mipLevels = mLevels;
    createInfo.tiling = tiling;
    createInfo.samples = samples;
    // TODO set queueFamilyIndexCount and pQueueFamilyIndices
    // createInfo.queueFamilyIndexCount =
    // createInfo.pQueueFamilyIndices =

    return create(dCtx, createInfo);
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

VkResult vulkanImage::bindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset) {
    assert(_device != VK_NULL_HANDLE);
    if (VkResult result = vkBindImageMemory(_device, _image, deviceMemory, memoryOffset)) {
        debug_error("::VULKAN:ERROR Failed to attach image to memory\n");
        return result;
    }
    return VK_SUCCESS;
}

bool vulkanImageView::create(const deviceContext& dCtx, const VkImageViewCreateInfo& createInfo) {
    assert(_image_view == VK_NULL_HANDLE && "::VULKAN:ERROR Can not recreate an image\n");
    _device = dCtx._device;

    VKCall(vkCreateImageView(_device, &createInfo, dCtx._allocator, &_image_view), "::VULKAN:ERROR Failed to create an image view\n");
    return true;
}

bool vulkanImageView::create(const deviceContext& dCtx, VkImage image, VkFormat format,
                                 VkImageViewType type, const VkComponentMapping& components, const VkImageSubresourceRange& range,
                                 VkImageViewCreateFlags flags, const void* next) {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.pNext = next;
    createInfo.flags = flags;
    createInfo.image = image;
    createInfo.viewType = type;
    createInfo.format = format;
    createInfo.components = components;
    createInfo.subresourceRange = range;

    return create(dCtx, createInfo);
}

void vulkanImageView::destroy() noexcept {
	if (_image_view != VK_NULL_HANDLE) {
        VK_DESTROY_PTR_BY(vkDestroyImageView, _device, _image_view);
        _device = VK_NULL_HANDLE;
	}
}

}
