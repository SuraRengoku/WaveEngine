#include "VulkanMemory.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {
/**
 * @brief adjust the range of non-host coherent memory when mapping memory
 * @param size mapping size
 * @param offset mapping offset of start position
 * @return actual start position of data in memory
 */
VkDeviceSize vulkanDeviceMemory::adjustNonCoherentMemoryRange(VkDeviceSize& size, VkDeviceSize& offset) const {
    const VkDeviceSize& nonCoherentAtomSize = VKX::findPhysicalDeviceProperties(CORE::physical_device()).limits.nonCoherentAtomSize;
    VkDeviceSize _offset = offset;
    // Down alignment offset (rounded down to a multiple of nonCoherentAtomSize)
    offset = offset / nonCoherentAtomSize * nonCoherentAtomSize;
    // Adjust: make sure covering the original range && Up alignment
    size = std::min((size + _offset + nonCoherentAtomSize - 1) / nonCoherentAtomSize * nonCoherentAtomSize, _allocation_size) - offset;
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
    if (!(_memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        inverseDeltaOffset = adjustNonCoherentMemoryRange(size, offset);
    }
    if (VkResult result = vkMapMemory(CORE::device(), _device_memory, offset, size, 0, &pData)) {
        debug_output("::VULKAN: Failed to map the memory\n");
        return result;
    }
    if (!(_memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        pData = static_cast<u8*>(pData) + inverseDeltaOffset;
        VkMappedMemoryRange mappedMemoryRange{};
        mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedMemoryRange.memory = _device_memory;
        mappedMemoryRange.offset = offset;
        mappedMemoryRange.size = size;

        if (VkResult result = vkInvalidateMappedMemoryRanges(CORE::device(), 1, &mappedMemoryRange)) {
            debug_output("::VULKAN: Failed to flush the memory\n");
            return result;
        }
    }
    return VK_SUCCESS;
}

VkResult vulkanDeviceMemory::unMapMemory(VkDeviceSize size, VkDeviceSize offset) const {
    if (!(_memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        adjustNonCoherentMemoryRange(size, offset);
        VkMappedMemoryRange mappedMemoryRange{};
        mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedMemoryRange.memory = _device_memory;
        mappedMemoryRange.offset = offset;
        mappedMemoryRange.size = size;

        if (VkResult result = vkFlushMappedMemoryRanges(CORE::device(), 1, &mappedMemoryRange)) {
            debug_output("::VULKAN: Failed to flush the memory\n");
            return result;
        }
    }
    vkUnmapMemory(CORE::device(), _device_memory);
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

VkResult vulkanDeviceMemory::allocate(VkMemoryAllocateInfo& allocateInfo) {
    if (allocateInfo.memoryTypeIndex >= VKX::findPhysicalDeviceMemoryProperties(CORE::physical_device()).memoryTypeCount) {
        debug_output("::VULKAN: Invalid memory type index\n");
        return VK_RESULT_MAX_ENUM;
    }
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    if (VkResult result = vkAllocateMemory(CORE::device(), &allocateInfo, nullptr, &_device_memory)) {
        debug_output("::VULKAN: Failed to allocate memory\n");
        return result;
    }
    _allocation_size = allocateInfo.allocationSize;
    _memory_properties = VKX::findPhysicalDeviceMemoryProperties(CORE::physical_device()).memoryTypes[allocateInfo.memoryTypeIndex].propertyFlags;
    return VK_SUCCESS;
}

VkResult vulkanBufferMemory::allocateMemory(VkMemoryPropertyFlags desiredMemoryProperties) {
    VkMemoryAllocateInfo allocateInfo = vulkanBuffer::memoryAllocateInfo(desiredMemoryProperties);
    if (allocateInfo.memoryTypeIndex >= VKX::findPhysicalDeviceMemoryProperties(CORE::physical_device()).memoryTypeCount) {
        return VK_RESULT_MAX_ENUM;
    }
    return vulkanDeviceMemory::allocate(allocateInfo);
}

VkResult vulkanBufferMemory::bindMemory() {
    if (VkResult result = vulkanBuffer::bindMemory(deviceMemory())) {
        return result;
    }
    areBound = true;
    return VK_SUCCESS;
}

/**
 * @brief allocate device memory, create buffer and bind
 * @param createInfo
 * @param desiredMemoryProperties
 * @return result
 */
VkResult vulkanBufferMemory::create(VkBufferCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties) {
    VkResult result;
    false ||
        (result = createBuffer(createInfo)) ||
        (result = allocateMemory(desiredMemoryProperties)) ||
        (result = bindMemory());
    return result;
}

VkResult vulkanImageMemory::allocateMemory(VkMemoryPropertyFlags desiredMemoryProperties) {
    VkMemoryAllocateInfo allocateInfo = vulkanImage::memoryAllocateInfo(desiredMemoryProperties);
    if (allocateInfo.memoryTypeIndex >= VKX::findPhysicalDeviceMemoryProperties(CORE::physical_device()).memoryTypeCount) {
        return VK_RESULT_MAX_ENUM;
    }
    return vulkanDeviceMemory::allocate(allocateInfo);
}

VkResult vulkanImageMemory::bindMemory() {
    if (VkResult result = vulkanImage::bindMemory(deviceMemory())) {
        return result;
    }
    areBound = false;
    return VK_SUCCESS;
}

VkResult vulkanImageMemory::create(VkImageCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties) {
    VkResult result;
    false ||
        (result = createImage(createInfo)) ||
        (result = allocateMemory(desiredMemoryProperties)) ||
        (result = bindMemory());
    return result;

}

}


