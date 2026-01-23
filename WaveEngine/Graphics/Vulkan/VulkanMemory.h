#pragma once
#include "VulkanBuffer.h"
#include "VulkanCommonHeaders.h"
#include "VulkanImage.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

struct PhysicalDeviceMemoryProperties {
    VkDeviceSize                        nonCoherentAtomSize{ 0 };
    VkPhysicalDeviceMemoryProperties    memoryProperties{};
};

enum class memoryMapAccess : u8 {
    Read,           // GPU -> CPU
    Write,          // CPU -> GPU
    ReadWrite
};

class vulkanDeviceMemory {
private:
    VkDeviceSize adjustNonCoherentMemoryRange(VkDeviceSize& size, VkDeviceSize& offset) const;

    VkDevice                            _device{ VK_NULL_HANDLE };
    VkDeviceMemory                      _device_memory{ VK_NULL_HANDLE };
    VkDeviceSize                        _allocation_size{ 0 };
    VkMemoryPropertyFlags               _memory_property_flags{ 0 };
    PhysicalDeviceMemoryProperties      _physical_device_properties{};

    // persistent mapping support
    mutable void*                       _mapped_ptr{ nullptr };
    mutable VkDeviceSize                _mapped_offset{ 0 };
    mutable VkDeviceSize                _mapped_size{ 0 };

public:
    vulkanDeviceMemory() = default;

    explicit vulkanDeviceMemory(VkDevice device) : _device(device) {}

    vulkanDeviceMemory(VkPhysicalDevice physicalDevice, VkDevice device, VkMemoryAllocateInfo& allocInfo);

    DISABLE_COPY(vulkanDeviceMemory);

    VK_MOVE_CTOR_CUSTOM(vulkanDeviceMemory,
        VK_MOVE_PTR(_device)
        VK_MOVE_PTR(_device_memory)
        VK_MOVE_PTR(_mapped_ptr)
        VK_MOVE_VALUE(_allocation_size)
        VK_MOVE_VALUE(_memory_property_flags)
        VK_MOVE_STRUCT(_physical_device_properties)
        VK_MOVE_VALUE(_mapped_offset)
        VK_MOVE_VALUE(_mapped_size)
    )

    vulkanDeviceMemory& operator=(vulkanDeviceMemory&& other) noexcept {
        if (this != &other) {
            if (_device_memory != VK_NULL_HANDLE) {
                VK_DESTROY_PTR_BY(vkFreeMemory, _device, _device_memory)
            }
            VK_MOVE_PTR(_device)
            VK_MOVE_PTR(_device_memory)
            VK_MOVE_VALUE(_allocation_size)
            VK_MOVE_VALUE(_memory_property_flags)
            VK_MOVE_STRUCT(_physical_device_properties)
            VK_MOVE_PTR(_mapped_ptr)
            VK_MOVE_VALUE(_mapped_offset)
            VK_MOVE_VALUE(_mapped_size)
        }
        return *this;
    }

    ~vulkanDeviceMemory() {
        if (_mapped_ptr != nullptr) {
            vkUnmapMemory(_device, _device_memory);
        }
        VK_DESTROY_PTR_BY(vkFreeMemory, _device, _device_memory)
    }

    [[nodiscard]] VK_DEFINE_PTR_TYPE_OPERATOR(_device_memory)
    [[nodiscard]] VK_DEFINE_ADDRESS_FUNCTION(_device_memory)

    [[nodiscard]] VkDeviceMemory deviceMemory() const { return _device_memory; }
    [[nodiscard]] VkDeviceSize allocationSize() const { return _allocation_size; }
    [[nodiscard]] VkMemoryPropertyFlags memoryProperties() const { return _memory_property_flags; }

    VkResult allocate(VkMemoryAllocateInfo& allocateInfo, VkPhysicalDevice physicalDevice);

    VkResult mapMemory(void*& pData, VkDeviceSize size, VkDeviceSize offset, memoryMapAccess access) const;
    VkResult unmapMemory(memoryMapAccess access) const;

    VkResult writeMemory(const void* src, VkDeviceSize size, VkDeviceSize offset = 0) const;
    VkResult readMemory(void* dst, VkDeviceSize size, VkDeviceSize offset = 0) const;

    // VkResult bufferData(const void* pData_src, VkDeviceSize size, VkDeviceSize offset = 0) const;
    //
    // template <typename T>
    // VkResult bufferData(const T& data_src) const {
    //     return bufferData(&data_src, sizeof data_src);
    // }

    // VkResult retrieveData(void* pData_dst, VkDeviceSize size, VkDeviceSize offset = 0) const;
};

class vulkanBufferMemory {
private:
    vulkanBuffer        _buffer;
    vulkanDeviceMemory  _memory;

public:
    vulkanBufferMemory() = default;
    DISABLE_COPY(vulkanBufferMemory)

    vulkanBufferMemory(vulkanBufferMemory&& other) noexcept
        : _buffer(std::move(other._buffer)), _memory(std::move(other._memory)) {
    }

    vulkanBufferMemory& operator=(vulkanBufferMemory&& other) noexcept {
        if (this != &other) {
            _buffer = std::move(other._buffer);
            _memory = std::move(other._memory);
        }
        return *this;
    }

    // Buffer access
    [[nodiscard]] VkBuffer buffer() const { return _buffer; }
    [[nodiscard]] const VkBuffer* addressOfBuffer() const { return _buffer.Address(); }

    // Memory access
    [[nodiscard]] VkDeviceMemory deviceMemory() const { return _memory; }
    [[nodiscard]] const VkDeviceMemory* addressOfDeviceMemory() const { return _memory.Address(); }

    VkDeviceSize allocationSize() const { return _memory.allocationSize(); }

    VkMemoryPropertyFlags memoryProperties() const { return _memory.memoryProperties();}

    // Memory operations
    VkResult mapMemory(void*& pData, VkDeviceSize size, VkDeviceSize offset, memoryMapAccess access) const {
        return _memory.mapMemory(pData, size, offset, access);
    }
    VkResult unMapMemory(memoryMapAccess access) const {
        return _memory.unmapMemory(access);
    }

    // write
    VkResult bufferData(const void* pData_src, VkDeviceSize size, VkDeviceSize offset = 0) const {
        return _memory.writeMemory(pData_src, size, offset);
    }
    template <typename T>
    VkResult bufferData(const T& data_src) const {
        return bufferData(&data_src, sizeof(T));
    }

    // read
    VkResult retrieveData(void* pData_dst, VkDeviceSize size, VkDeviceSize offset = 0) const {
        return _memory.readMemory(pData_dst, size, offset);
    }

    VkResult create(VkDevice device, VkPhysicalDevice physicalDevice, const VkBufferCreateInfo& bufferCreateInfo,
                    VkMemoryPropertyFlags desiredMemoryProperties);
};

class vulkanImageMemory {
private:
    vulkanImage         _image;
    vulkanDeviceMemory  _memory;

public:
    vulkanImageMemory() = default;

    DISABLE_COPY(vulkanImageMemory)

    vulkanImageMemory(vulkanImageMemory&& other) noexcept
        : _image(std::move(other._image)), _memory(std::move(other._memory)) {
    }

    vulkanImageMemory& operator=(vulkanImageMemory&& other) noexcept {
        if (this != &other) {
            _image = std::move(other._image);
            _memory = std::move(other._memory);
        }
        return *this;
    }

    // Image access
    [[nodiscard]] VkImage image() const { return _image; }
    [[nodiscard]] const VkImage* addressOfImage() const { return _image.Address(); }

    // Memory access
    [[nodiscard]] VkDeviceMemory deviceMemory() const { return _memory; }
    [[nodiscard]] const VkDeviceMemory* addressOfDeviceMemory() const { return _memory.Address(); }

    VkDeviceSize allocationSize() const { return _memory.allocationSize(); }

    VkMemoryPropertyFlags memoryProperties() const { return _memory.memoryProperties(); }

    VkResult create(VkDevice device, VkPhysicalDevice physicalDevice, const VkImageCreateInfo& imageCreateInfo,
                    VkMemoryPropertyFlags desiredMemoryProperties);
};

} // namespace WAVEENGINE::GRAPHICS::VULKAN
