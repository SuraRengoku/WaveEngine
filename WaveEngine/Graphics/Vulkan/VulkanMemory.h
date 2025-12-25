#pragma once
#include "VulkanBuffer.h"
#include "VulkanCommonHeaders.h"
#include "VulkanCore.h"
#include "VulkanImage.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

struct PhysicalDeviceMemoryProperties {
    VkDeviceSize                        nonCoherentAtomSize{ 0 };
    VkPhysicalDeviceMemoryProperties    memoryProperties{};
};

class vulkanDeviceMemory {
private:
    VkDeviceSize adjustNonCoherentMemoryRange(VkDeviceSize& size, VkDeviceSize& offset) const;

    VkDevice                            _device{ VK_NULL_HANDLE };
    VkDeviceMemory                      _device_memory{ VK_NULL_HANDLE };
    VkDeviceSize                        _allocation_size{ 0 };
    VkMemoryPropertyFlags               _memory_property_flags{ 0 };
    PhysicalDeviceMemoryProperties      _physical_device_properties{};

public:
    vulkanDeviceMemory() = default;
    explicit vulkanDeviceMemory(VkDevice device) : _device(device) {}
    vulkanDeviceMemory(VkPhysicalDevice physicalDevice, VkDevice device, VkMemoryAllocateInfo& allocInfo);


    DISABLE_COPY(vulkanDeviceMemory);

    vulkanDeviceMemory(vulkanDeviceMemory&& other) noexcept {
        VK_MOVE_PTR(_device);
        VK_MOVE_PTR(_device_memory);
        VK_MOVE_VALUE(_allocation_size);
        VK_MOVE_VALUE(_memory_property_flags);
        VK_MOVE_STRUCT(_physical_device_properties);
    }
    vulkanDeviceMemory& operator=(vulkanDeviceMemory&& other) noexcept {
        if (this != &other) {
            if (_device_memory != VK_NULL_HANDLE) {
                VK_DESTROY_PTR_BY(vkFreeMemory, _device, _device_memory);
            }
            VK_MOVE_PTR(_device);
            VK_MOVE_PTR(_device_memory);
            VK_MOVE_VALUE(_allocation_size);
            VK_MOVE_VALUE(_memory_property_flags);
            VK_MOVE_STRUCT(_physical_device_properties);
        }
        return *this;
    }

    ~vulkanDeviceMemory() {
        VK_DESTROY_PTR_BY(vkFreeMemory, _device, _device_memory);
    }

    VK_DEFINE_PTR_TYPE_OPERATOR(_device_memory);
    VK_DEFINE_ADDRESS_FUNCTION(_device_memory);

    VkDevice device() const { return _device; }
    VkDeviceSize allocationSize() const { return _allocation_size; }

    VkMemoryPropertyFlags memoryProperties() const {
        return _memory_property_flags;
    }

    VkResult mapMemory(void*& pData, VkDeviceSize size, VkDeviceSize offset = 0) const;
    VkResult unMapMemory(VkDeviceSize size, VkDeviceSize offset = 0) const;
    VkResult bufferData(const void* pData_src, VkDeviceSize size, VkDeviceSize offset = 0) const;

    template <typename T>
    VkResult bufferData(const T& data_src) const {
        return bufferData(&data_src, sizeof data_src);
    }

    VkResult retrieveData(void* pData_dst, VkDeviceSize size, VkDeviceSize offset = 0) const;
    VkResult allocate(VkMemoryAllocateInfo& allocateInfo, VkPhysicalDevice physicalDevice);
};

class vulkanBufferMemory {
private:
    vulkanBuffer        _buffer;
    vulkanDeviceMemory  _memory;

public:
    vulkanBufferMemory() = default;
    DISABLE_COPY(vulkanBufferMemory);

    vulkanBufferMemory(vulkanBufferMemory&& other) noexcept
        : _buffer(std::move(other._buffer)), _memory(std::move(other._memory)) {
    }

    // Buffer access
    VkBuffer buffer() const { return _buffer; }
    const VkBuffer* addressOfBuffer() const { return _buffer.Address(); }

    // Memory access
    VkDeviceMemory deviceMemory() const { return _memory; }

    const VkDeviceMemory* addressOfDeviceMemory() const {
        return _memory.Address();
    }

    VkDeviceSize allocationSize() const { return _memory.allocationSize(); }

    VkMemoryPropertyFlags memoryProperties() const {
        return _memory.memoryProperties();
    }

    // Memory operations
    VkResult mapMemory(void*& pData, VkDeviceSize size, VkDeviceSize offset = 0) const {
        return _memory.mapMemory(pData, size, offset);
    }

    VkResult unMapMemory(VkDeviceSize size, VkDeviceSize offset = 0) const {
        return _memory.unMapMemory(size, offset);
    }

    VkResult bufferData(const void* pData_src, VkDeviceSize size, VkDeviceSize offset = 0) const {
        return _memory.bufferData(pData_src, size, offset);
    }

    template <typename T>
    VkResult bufferData(const T& data_src) const {
        return _memory.bufferData(data_src);
    }

    VkResult retrieveData(void* pData_dst, VkDeviceSize size, VkDeviceSize offset = 0) const {
        return _memory.retrieveData(pData_dst, size, offset);
    }

    // Creation workflow
    VkResult create(VkDevice device, VkPhysicalDevice physicalDevice, VkBufferCreateInfo& bufferCreateInfo,
                    VkMemoryPropertyFlags desiredMemoryProperties);
};

class vulkanImageMemory {
private:
    vulkanImage         _image;
    vulkanDeviceMemory  _memory;

public:
    vulkanImageMemory() = default;
    DISABLE_COPY(vulkanImageMemory);

    vulkanImageMemory(vulkanImageMemory&& other) noexcept
        : _image(std::move(other._image)), _memory(std::move(other._memory)) {
    }

    // Image access
    VkImage image() const { return _image; }
    const VkImage* addressOfImage() const { return _image.Address(); }

    // Memory access
    VkDeviceMemory deviceMemory() const { return _memory; }

    const VkDeviceMemory* addressOfDeviceMemory() const {
        return _memory.Address();
    }

    VkDeviceSize allocationSize() const { return _memory.allocationSize(); }

    VkMemoryPropertyFlags memoryProperties() const {
        return _memory.memoryProperties();
    }

    // Creation workflow
    VkResult create(VkDevice device, VkPhysicalDevice physicalDevice, VkImageCreateInfo& imageCreateInfo,
                    VkMemoryPropertyFlags desiredMemoryProperties);
};

} // namespace WAVEENGINE::GRAPHICS::VULKAN
