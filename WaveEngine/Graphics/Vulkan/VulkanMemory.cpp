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
    VkDeviceSize _offset = offset;
    // Down alignment offset (rounded down to a multiple of nonCoherentAtomSize)
    offset = offset / nonCoherentAtomSize * nonCoherentAtomSize;
    // Adjust: make sure covering the original range && Up alignment
    size = std::min((size + _offset + nonCoherentAtomSize - 1) /
                    nonCoherentAtomSize * nonCoherentAtomSize, _allocation_size) - offset;
    return _offset - offset;
}

/**
 * @brief map host-visible memory
 * @param pData
 * @param size
 * @param offset
 * @return result
 */
VkResult vulkanDeviceMemory::mapMemory(void*& pData, VkDeviceSize size, VkDeviceSize offset) const {
    VkDeviceSize inverseDeltaOffset = 0;
    if (!(_memory_property_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        inverseDeltaOffset = adjustNonCoherentMemoryRange(size, offset);
    }
    if (VkResult result = vkMapMemory(_device, _device_memory, offset, size, 0, &pData)) {
        debug_error("::VULKAN:ERROR Failed to map the memory\n");
        return result;
    }
    if (!(_memory_property_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        pData = static_cast<u8*>(pData) + inverseDeltaOffset;
        VkMappedMemoryRange mappedMemoryRange{};
        mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedMemoryRange.memory = _device_memory;
        mappedMemoryRange.offset = offset;
        mappedMemoryRange.size = size;

        assert(_device != VK_NULL_HANDLE);
        if (VkResult result = vkInvalidateMappedMemoryRanges(_device, 1, &mappedMemoryRange)) {
            debug_error("::VULKAN:ERROR Failed to flush the memory\n");
            return result;
        }
    }
    return VK_SUCCESS;
}

VkResult vulkanDeviceMemory::unMapMemory(VkDeviceSize size, VkDeviceSize offset) const {
    if (!(_memory_property_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        adjustNonCoherentMemoryRange(size, offset);
        VkMappedMemoryRange mappedMemoryRange{};
        mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedMemoryRange.memory = _device_memory;
        mappedMemoryRange.offset = offset;
        mappedMemoryRange.size = size;

        if (VkResult result = vkFlushMappedMemoryRanges(_device, 1, &mappedMemoryRange)) {
            debug_error("::VULKAN:ERROR Failed to flush the memory\n");
            return result;
        }
    }
    vkUnmapMemory(_device, _device_memory);
    return VK_SUCCESS;
}

VkResult vulkanDeviceMemory::bufferData(const void* pData_src, VkDeviceSize size, VkDeviceSize offset) const {
    void* pData_dst;
    if (VkResult result = mapMemory(pData_dst, size, offset))
        return result;

    memcpy(pData_dst, pData_src, size_t(size));
    return unMapMemory(size, offset);
}

VkResult vulkanDeviceMemory::retrieveData(void* pData_dst, VkDeviceSize size, VkDeviceSize offset) const {
    void* pData_src;
    if (VkResult result = mapMemory(pData_src, size, offset))
        return result;
    memcpy(pData_dst, pData_src, size_t(size));
    return unMapMemory(size, offset);
}

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

///////////////////////////////////////////////////// VULKAN BUFFER MEMORY //////////////////////////////////////////////////////

VkResult vulkanBufferMemory::create(VkDevice device, VkPhysicalDevice physicalDevice, VkBufferCreateInfo& bufferCreateInfo, VkMemoryPropertyFlags desiredMemoryProperties) {
    // Set device for both buffer and memory
    _buffer.setDevice(device);
    _memory = std::move(vulkanDeviceMemory(device));

    // Create buffer
    if (VkResult result = _buffer.create(bufferCreateInfo)) {
        return result;
    }

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
        return result;
    }

    // Bind buffer to memory
    if (VkResult result = _buffer.bindMemory(_memory)) {
        return result;
    }

    return VK_SUCCESS;
}

///////////////////////////////////////////////////// VULKAN IMAGE MEMORY //////////////////////////////////////////////////////

VkResult vulkanImageMemory::create(VkDevice device, VkPhysicalDevice physicalDevice, VkImageCreateInfo& imageCreateInfo, VkMemoryPropertyFlags desiredMemoryProperties) {
    // Set device for both image and memory
    _image.setDevice(device);
    _memory = std::move(vulkanDeviceMemory(device));

    // Create image
    if (VkResult result = _image.create(imageCreateInfo)) {
        return result;
    }

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
        return result;
    }

    // Bind image to memory
    if (VkResult result = _image.bindMemory(_memory)) {
        return result;
    }

    return VK_SUCCESS;
}
} // namespace WAVEENGINE::GRAPHICS::VULKAN
