#include "VulkanBuffer.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

void vulkanBuffer::setDevice(VkDevice device) {
    assert(_device == VK_NULL_HANDLE);
    _device = device;
}

VkMemoryAllocateInfo vulkanBuffer::memoryAllocateInfo(VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags desiredMemoryProperties) const {
    assert(_device != VK_NULL_HANDLE);
    VkMemoryAllocateInfo memoryAllocateInfo{};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(_device, _buffer, &memoryRequirements);
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = UINT32_MAX;
    const auto& physicalDeviceMemoryProperties = VKX::findPhysicalDeviceMemoryProperties(physicalDevice);
    for (size_t i{0}; i < physicalDeviceMemoryProperties.memoryProperties.memoryTypeCount; ++i) {
        if (memoryRequirements.memoryTypeBits & 1 << i
            && (physicalDeviceMemoryProperties.memoryProperties.memoryTypes[i].propertyFlags & desiredMemoryProperties) == desiredMemoryProperties) {
            memoryAllocateInfo.memoryTypeIndex = i;
            break;
        }
    }
    // since we return memoryAllocateInfo out of this function, remember to check it when call this function
    return memoryAllocateInfo;
}

VkResult vulkanBuffer::bindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset) const {
    assert(_device != VK_NULL_HANDLE);
    if (VkResult result = vkBindBufferMemory(_device, _buffer, deviceMemory, memoryOffset)) {
        debug_output("::VULKAN: Failed to attach buffer to memory\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanBuffer::create(VkBufferCreateInfo& createInfo) {
    assert(_device != VK_NULL_HANDLE);
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    if (VkResult result = vkCreateBuffer(_device, &createInfo, nullptr, &_buffer)) {
        debug_output("::VULKAN: Failed to create a buffer\n");
        return result;
    }
    return VK_SUCCESS;
}

void vulkanBufferView::setDevice(VkDevice device) {
    assert(_device == VK_NULL_HANDLE);
    _device = device;
}

VkResult vulkanBufferView::create(const VkBufferViewCreateInfo& createInfo) {
    assert(_device != VK_NULL_HANDLE);
    if (VkResult result = vkCreateBufferView(_device, &createInfo, nullptr, &_buffer_view)) {
        debug_output("::VULKAN: Failed to create a buffer view\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanBufferView::create(VkBuffer buffer, VkFormat format, VkDeviceSize offset, VkDeviceSize range) {
    VkBufferViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    createInfo.format = format;
    createInfo.buffer = buffer;
    createInfo.offset = offset;
    createInfo.range = range;
    return create(createInfo);
}

}
