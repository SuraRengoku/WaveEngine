#pragma once
#include "VulkanCommonHeaders.h"
#include "VulkanCore.h"
#include "VulkanBuffer.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

class vulkanDeviceMemory {
private:
    VkDeviceSize adjustNonCoherentMemoryRange(VkDeviceSize& size, VkDeviceSize& offset) const;

    VkDeviceMemory                  _device_memory{ VK_NULL_HANDLE };
    VkDeviceSize                    _allocation_size{0};
    VkMemoryPropertyFlags           _memory_properties{0};

public:
    vulkanDeviceMemory() = default;
    vulkanDeviceMemory(VkMemoryAllocateInfo& allocInfo) {
        allocate(allocInfo);
    }
    DISABLE_COPY(vulkanDeviceMemory);
    vulkanDeviceMemory(vulkanDeviceMemory&& other) noexcept {
        VK_MOVE_PTR(_device_memory);
        VK_MOVE_VALUE(_allocation_size);
        VK_MOVE_VALUE(_memory_properties);
    }
    ~vulkanDeviceMemory() {
        VK_DESTROY_PTR_BY(vkFreeMemory, CORE::device(), _device_memory);
        _allocation_size = 0;
        _memory_properties = 0;
    }

    VK_DEFINE_PTR_TYPE_OPERATOR(_device_memory);
    VK_DEFINE_ADDRESS_FUNCTION(_device_memory);

    VkDeviceSize allocationSize() const { return _allocation_size; }
    VkMemoryPropertyFlags memoryProperties() const { return _memory_properties; }

    VkResult mapMemory(void*& pData, VkDeviceSize size, VkDeviceSize offset = 0) const;
    VkResult unMapMemory(VkDeviceSize size, VkDeviceSize offset = 0) const;
    VkResult bufferData(const void* pData_src, VkDeviceSize size, VkDeviceSize offset = 0) const;
    template<typename T>
    VkResult bufferData(const T& data_src) const {
        return bufferData(&data_src, sizeof data_src);
    }

    VkResult retrieveData(void* pData_dst, VkDeviceSize size, VkDeviceSize offset = 0) const;
    VkResult allocate(VkMemoryAllocateInfo& allocateInfo);

protected:
    class {
        friend class bufferMemory;
        friend class imageMemory;
        bool value = false;
        operator bool() const { return value; }
        auto& operator=(bool value) { this->value = value; return *this; }
    } areBound;
};

class vulkanBufferMemory : vulkanBuffer, vulkanDeviceMemory {
public:
    vulkanBufferMemory() = default;
    DISABLE_COPY(vulkanBufferMemory);
    vulkanBufferMemory(VkBufferCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties) {
        create(createInfo, desiredMemoryProperties);
    }
    vulkanBufferMemory(vulkanBufferMemory&& other) noexcept : vulkanBuffer(std::move(other)), vulkanDeviceMemory(std::move(other)) {
        areBound = other.areBound;
        other.areBound = false;
    }
    ~vulkanBufferMemory() { areBound = false; }

    //
    VkBuffer buffer() const { return static_cast<const vulkanBuffer&>(*this); }
    const VkBuffer* addressOfBuffer() const { return vulkanBuffer::Address(); }
    VkDeviceMemory deviceMemory() const { return static_cast<const vulkanDeviceMemory&>(*this); }
    const VkDeviceMemory* addressOfDeviceMemory() const { return vulkanDeviceMemory::Address(); }

    // if areBound is true, we have already successfully allocated device memory and created buffer and also bound together
    bool are_bound() const {return areBound;}
    using vulkanDeviceMemory::allocationSize;
    using vulkanDeviceMemory::memoryProperties;

    using vulkanDeviceMemory::mapMemory;
    using vulkanDeviceMemory::unMapMemory;
    using vulkanDeviceMemory::bufferData;
    using vulkanDeviceMemory::retrieveData;

    VkResult createBuffer(VkBufferCreateInfo& createInfo) {
        return vulkanBuffer::create(createInfo);
    }

    VkResult allocateMemory(VkMemoryPropertyFlags desiredMemoryProperties);

    VkResult bindMemory();

    VkResult create(VkBufferCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties);
};

}