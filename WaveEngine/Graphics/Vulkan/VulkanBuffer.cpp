#include "VulkanBuffer.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

VkMemoryAllocateInfo vulkanBuffer::memoryAllocateInfo(VkMemoryPropertyFlags desiredMemoryProperties) const {
    VkMemoryAllocateInfo memoryAllocateInfo{};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(CORE::device(), _buffer, &memoryRequirements);
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = UINT32_MAX;
    const auto& physicalDeviceMemoryProperties = VKX::findPhysicalDeviceMemoryProperties(CORE::physical_device());
    for (size_t i{0}; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i) {
        if (memoryRequirements.memoryTypeBits & 1 << i
            && (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & desiredMemoryProperties) == desiredMemoryProperties) {
            memoryAllocateInfo.memoryTypeIndex = i;
            break;
        }
    }
    // since we return memoryAllocateInfo out of this function, remember to check it when call this function
    return memoryAllocateInfo;
}

VkResult vulkanBuffer::bindMemory(VkDeviceMemory device_memory, VkDeviceSize memoryOffset) const {
    if (VkResult result = vkBindBufferMemory(CORE::device(), _buffer, device_memory, memoryOffset)) {
        debug_output("::VULKAN: Failed to attach buffer to memory\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanBuffer::create(VkBufferCreateInfo& createInfo) {
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    if (VkResult result = vkCreateBuffer(CORE::device(), &createInfo, nullptr, &_buffer)) {
        debug_output("::VULKAN: Failed to create a buffer\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanBufferView::create(VkBufferViewCreateInfo& createInfo) {
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    if (VkResult result = vkCreateBufferView(CORE::device(), &createInfo, nullptr, &_buffer_view)) {
        debug_output("::VULKAN: Failed to create a buffer view\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanBufferView::create(VkBuffer buffer, VkFormat format, VkDeviceSize offset, VkDeviceSize range) {
    VkBufferViewCreateInfo createInfo{};
    createInfo.format = format;
    createInfo.buffer = buffer;
    createInfo.offset = offset;
    createInfo.range = range;
    return create(createInfo);
}

}
