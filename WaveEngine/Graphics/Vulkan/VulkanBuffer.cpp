#include "VulkanBuffer.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

bool vulkanBuffer::create(const deviceContext& dCtx, const VkBufferCreateInfo& createInfo) {
    assert(_buffer == VK_NULL_HANDLE && "::VULKAN:ERROR Can not recreate a buffer\n");
    _device = dCtx._device;

    VKCall(vkCreateBuffer(_device, &createInfo, dCtx._allocator, &_buffer), "::VULKAN:ERROR Failed to create a buffer\n");
    return true;
}

bool vulkanBuffer::create(const deviceContext& dCtx, VkSharingMode sMode, VkDeviceSize dSize,
                          const VkBufferCreateFlags cFlags, const VkBufferUsageFlags uFlags, const void* next) {

    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.flags = cFlags;
    createInfo.usage = uFlags;
    createInfo.pNext = next;
    createInfo.sharingMode = sMode;
    createInfo.size = dSize;
    // TODO set queueFamilyIndexCount and pQueueFamilyIndices
    // createInfo.queueFamilyIndexCount =
    // createInfo.pQueueFamilyIndices =

    return create(dCtx, createInfo);
}

VkMemoryAllocateInfo vulkanBuffer::memoryAllocateInfo(VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags desiredMemoryProperties) const {
    assert(_device != VK_NULL_HANDLE && _buffer != VK_NULL_HANDLE);

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(_device, _buffer, &memoryRequirements);

    VkMemoryAllocateInfo memoryAllocateInfo{};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
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

VkResult vulkanBuffer::bindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset) {
    assert(_device != VK_NULL_HANDLE && _buffer != VK_NULL_HANDLE);
    if (VkResult result = vkBindBufferMemory(_device, _buffer, deviceMemory, memoryOffset)) {
        debug_error("::VULKAN:ERROR Failed to attach buffer to memory\n");
        return result;
    }
    return VK_SUCCESS;
}

bool vulkanBufferView::create(const deviceContext& dCtx, const VkBufferViewCreateInfo& createInfo) {
    assert(_buffer_view == VK_NULL_HANDLE && "::VULKAN:ERROR Can not recreate a buffer view\n");
    _device = dCtx._device;

    VKCall(vkCreateBufferView(_device, &createInfo, dCtx._allocator, &_buffer_view), "::VULKAN:ERROR Failed to create a buffer view\n");
    return true;
}

bool vulkanBufferView::create(const deviceContext& dCtx, VkBuffer buffer, VkFormat format, VkDeviceSize offset,
                              VkDeviceSize range, VkBufferViewCreateFlags flags, const void* next) {

    VkBufferViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    createInfo.flags = flags;
    createInfo.pNext = next;
    createInfo.buffer = buffer;
    createInfo.format = format;
    createInfo.offset = offset;
    createInfo.range = range;

    return create(dCtx, createInfo);
}

}
