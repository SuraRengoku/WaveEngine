#include "VulkanMemory.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {
///////////////////////////////////////////////////// VULKAN DEVICE MEMORY //////////////////////////////////////////////////////

vulkanDeviceMemory::vulkanDeviceMemory(VkPhysicalDevice physicalDevice,
                                       VkDevice device,
                                       VkMemoryAllocateInfo& allocInfo)
    : _device(device) {
    _physical_device_properties.nonCoherentAtomSize = VKX::findPhysicalDeviceProperties(physicalDevice).properties.limits.nonCoherentAtomSize;
    _physical_device_properties.memoryProperties = VKX::findPhysicalDeviceMemoryProperties(physicalDevice).memoryProperties;
    allocate(allocInfo, physicalDevice);
}

/**
 * @brief adjust the range of non-host coherent memory when mapping memory
 * @param size mapping size
 * @param offset mapping offset of start position
 * @return actual start position of data in memory
 */
VkDeviceSize vulkanDeviceMemory::adjustNonCoherentMemoryRange(VkDeviceSize& size, VkDeviceSize& offset) const {
    assert(_physical_device_properties.nonCoherentAtomSize != 0);
    const VkDeviceSize& nonCoherentAtomSize = _physical_device_properties.nonCoherentAtomSize;
    assert(nonCoherentAtomSize > 0);

    VkDeviceSize originalOffset = offset;
    VkDeviceSize alignedOffset = (offset / nonCoherentAtomSize) * nonCoherentAtomSize; // Down alignment offset (rounded down to a multiple of nonCoherentAtomSize)

    // Adjust: make sure covering the original range && Up alignment
    VkDeviceSize end = std::min(originalOffset + size, _allocation_size);
    VkDeviceSize alignedEnd = ((end + nonCoherentAtomSize - 1) / nonCoherentAtomSize) * nonCoherentAtomSize;

    offset = alignedOffset;
    size = alignedEnd - alignedOffset;

    return originalOffset - alignedOffset;
}

/**
 * @brief map host-visible memory
 * @param pData
 * @param size
 * @param offset
 * @return result
 */
VkResult vulkanDeviceMemory::mapMemory(void*& pData, VkDeviceSize size, VkDeviceSize offset, memoryMapAccess access) const {
    // persistent reuse
    if (_mapped_ptr) {
        pData = static_cast<u8*>(_mapped_ptr) + (offset - _mapped_offset);
        return VK_SUCCESS;
    }

    VkDeviceSize delta = 0;
    VkDeviceSize mapOffset = offset;
    VkDeviceSize mapSize = size;

    if (!(_memory_property_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        delta = adjustNonCoherentMemoryRange(mapSize, mapOffset);
    }

    void* ptr = nullptr;
    if (VkResult result = vkMapMemory(_device, _device_memory, mapOffset, mapSize, 0, &ptr)) {
        debug_error("::VULKAN:ERROR Failed to map the memory\n");
        return result;
    }

    // GPU -> CPU
    if (access == memoryMapAccess::Read || access == memoryMapAccess::ReadWrite) {
        if (!(_memory_property_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            VkMappedMemoryRange mappedMemoryRange{};
            mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            mappedMemoryRange.memory = _device_memory;
            mappedMemoryRange.offset = mapOffset;
            mappedMemoryRange.size = mapSize;

            vkInvalidateMappedMemoryRanges(_device, 1, &mappedMemoryRange);
        }
    }

    _mapped_ptr     = ptr;
    _mapped_offset  = mapOffset;
    _mapped_size    = mapSize;

    pData = static_cast<u8*>(ptr) + delta;
    return VK_SUCCESS;
}

VkResult vulkanDeviceMemory::unmapMemory(memoryMapAccess access) const {
    if (!_mapped_ptr) return VK_SUCCESS;

    // CPU -> GPU
    if (access == memoryMapAccess::Write || access == memoryMapAccess::ReadWrite) {
        if (!(_memory_property_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            VkMappedMemoryRange mappedMemoryRange{};
            mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            mappedMemoryRange.memory = _device_memory;
            mappedMemoryRange.offset = _mapped_offset;
            mappedMemoryRange.size = _mapped_size;

            vkFlushMappedMemoryRanges(_device, 1, &mappedMemoryRange);
        }
    }

    vkUnmapMemory(_device, _device_memory);
    _mapped_ptr = nullptr;
    return VK_SUCCESS;
}

// VkResult vulkanDeviceMemory::bufferData(const void* pData_src, VkDeviceSize size, VkDeviceSize offset) const {
//     void* pData_dst;
//     if (VkResult result = mapMemory(pData_dst, size, offset))
//         return result;
//
//     memcpy(pData_dst, pData_src, size_t(size));
//     return unMapMemory(size, offset);
// }
//
// VkResult vulkanDeviceMemory::retrieveData(void* pData_dst, VkDeviceSize size, VkDeviceSize offset) const {
//     void* pData_src;
//     if (VkResult result = mapMemory(pData_src, size, offset))
//         return result;
//     memcpy(pData_dst, pData_src, size_t(size));
//     return unMapMemory(size, offset);
// }

VkResult vulkanDeviceMemory::allocate(VkMemoryAllocateInfo& allocateInfo, VkPhysicalDevice physicalDevice) {
    // Initialize physical device properties if not set
    if (_physical_device_properties.nonCoherentAtomSize == 0) {
        _physical_device_properties.nonCoherentAtomSize = VKX::findPhysicalDeviceProperties(physicalDevice) .properties.limits.nonCoherentAtomSize;
        _physical_device_properties.memoryProperties = VKX::findPhysicalDeviceMemoryProperties(physicalDevice).memoryProperties;
    }

    if (allocateInfo.memoryTypeIndex >= _physical_device_properties.memoryProperties.memoryTypeCount) {
        debug_error("::VULKAN:ERROR Invalid memory type index\n");
        return VK_RESULT_MAX_ENUM;
    }
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    if (VkResult result = vkAllocateMemory(_device, &allocateInfo, nullptr, &_device_memory)) {
        debug_error("::VULKAN:ERROR Failed to allocate memory\n");
        return result;
    }
    _allocation_size = allocateInfo.allocationSize;
    _memory_property_flags = _physical_device_properties.memoryProperties.memoryTypes[allocateInfo.memoryTypeIndex].propertyFlags;
    return VK_SUCCESS;
}

VkResult vulkanDeviceMemory::writeMemory(const void* src, VkDeviceSize size, VkDeviceSize offset) const {
    void* dst = nullptr;
    if (VkResult result = mapMemory(dst, size, offset, memoryMapAccess::Write)) {
        debug_error("::VULKAN:ERROR Failed to write memory\n");
        return result;
    }

    memcpy(dst, src, size);
    return unmapMemory(memoryMapAccess::Write);
}

VkResult vulkanDeviceMemory::readMemory(void* dst, VkDeviceSize size, VkDeviceSize offset) const {
    void* src = nullptr;
    if (VkResult result = mapMemory(src, size, offset, memoryMapAccess::Read)) {
        debug_error("::VULKAN:ERROR Failed to read memory\n");
        return result;
    }

    memcpy(dst, src, size);
    return unmapMemory(memoryMapAccess::Read);
}

///////////////////////////////////////////////////// VULKAN BUFFER MEMORY //////////////////////////////////////////////////////

VkResult vulkanBufferMemory::create(VkDevice device, VkPhysicalDevice physicalDevice, const VkBufferCreateInfo& bufferCreateInfo, VkMemoryPropertyFlags desiredMemoryProperties) {
    // Set device for both buffer and memory
    _memory = std::move(vulkanDeviceMemory(device));
    // Create buffer
    VKbCall(_buffer.create({device, {}, {}, nullptr}, bufferCreateInfo), "::VULKAN:ERROR Can not create buffer memory\n");

    // Allocate memory
    VkMemoryAllocateInfo allocateInfo =
        _buffer.memoryAllocateInfo(physicalDevice, desiredMemoryProperties);
    if (allocateInfo.memoryTypeIndex >=
        VKX::findPhysicalDeviceMemoryProperties(physicalDevice)
        .memoryProperties.memoryTypeCount) {
        debug_error("::VULKAN:ERROR Invalid memory type index for buffer\n");
        return VK_RESULT_MAX_ENUM;
    }

    if (VkResult result = _memory.allocate(allocateInfo, physicalDevice)) {
        debug_error("::VULKAN:ERROR Can not allocate memory to buffer\n");
        return result;
    }

    // Bind buffer to memory
    if (VkResult result = _buffer.bindMemory(_memory)) {
        debug_error("::VULKAN:ERROR Can not bind buffer to memory\n");
        return result;
    }

    return VK_SUCCESS;
}

///////////////////////////////////////////////////// VULKAN IMAGE MEMORY //////////////////////////////////////////////////////

VkResult vulkanImageMemory::create(VkDevice device, VkPhysicalDevice physicalDevice, const VkImageCreateInfo& imageCreateInfo, VkMemoryPropertyFlags desiredMemoryProperties) {
    // Set device for both image and memory
    _memory = std::move(vulkanDeviceMemory(device));
    // Create image
    VKbCall(_image.create({device, {}, {}, nullptr}, imageCreateInfo), "::VULKAN:ERROR Can not create image memory\n");

    // Allocate memory
    VkMemoryAllocateInfo allocateInfo =
        _image.memoryAllocateInfo(physicalDevice, desiredMemoryProperties);
    if (allocateInfo.memoryTypeIndex >=
        VKX::findPhysicalDeviceMemoryProperties(physicalDevice)
        .memoryProperties.memoryTypeCount) {
        debug_error("::VULKAN:ERROR Invalid memory type index for image\n");
        return VK_RESULT_MAX_ENUM;
    }

    if (VkResult result = _memory.allocate(allocateInfo, physicalDevice)) {
        debug_error("::VULKAN:ERROR Can not allocate memory to image\n");
        return result;
    }

    // Bind image to memory
    if (VkResult result = _image.bindMemory(_memory)) {
        debug_error("::VULKAN:ERROR Can not bind image to memory\n");
        return result;
    }

    return VK_SUCCESS;
}

} // namespace WAVEENGINE::GRAPHICS::VULKAN
