#pragma once
#include "VulkanCommonHeaders.h"
#include "VulkanCore.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

class vulkanBuffer {
private:
    VkBuffer                _buffer{ VK_NULL_HANDLE };
    VkDevice                _device{ VK_NULL_HANDLE };

public:
    vulkanBuffer() = default;
    DISABLE_COPY(vulkanBuffer);
    vulkanBuffer(VkDevice device) : _device(device) {}
    vulkanBuffer(VkDevice device, VkBufferCreateInfo& createInfo) : _device(device) {
        create(createInfo);
    }
    vulkanBuffer(vulkanBuffer&& other) noexcept {
        VK_MOVE_PTR(_buffer);
        VK_MOVE_PTR(_device);
    }
    ~vulkanBuffer() {
        VK_DESTROY_PTR_BY(vkDestroyBuffer, _device, _buffer);
    }

    VK_DEFINE_PTR_TYPE_OPERATOR(_buffer);
    VK_DEFINE_ADDRESS_FUNCTION(_buffer);

    // delayed injection
    void setDevice(VkDevice device);

    VkMemoryAllocateInfo memoryAllocateInfo(VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags desiredMemoryProperties) const;

    VkResult bindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset = 0) const;

    VkResult create(VkBufferCreateInfo& createInfo);
};

class vulkanBufferView {
private:
    VkBufferView            _buffer_view{ VK_NULL_HANDLE };
    VkDevice                _device{ VK_NULL_HANDLE };

public:
    vulkanBufferView() = default;
    DISABLE_COPY(vulkanBufferView);
    vulkanBufferView(VkDevice device) : _device(device) {}
    vulkanBufferView(VkDevice device, const VkBufferViewCreateInfo& createInfo) : _device(device) {
        create(createInfo);
    }
    vulkanBufferView(VkBuffer buffer, VkFormat format, VkDeviceSize offset = 0, VkDeviceSize range = 0 /*, VkBufferViewCreateFlags flags*/) {
        create(buffer, format, offset, range);
    }
    vulkanBufferView(vulkanBufferView&& other) noexcept {
        VK_MOVE_PTR(_buffer_view);
        VK_MOVE_PTR(_device);
    }
    ~vulkanBufferView() {
        VK_DESTROY_PTR_BY(vkDestroyBufferView, _device, _buffer_view);
    }

    VK_DEFINE_PTR_TYPE_OPERATOR(_buffer_view);
    VK_DEFINE_ADDRESS_FUNCTION(_buffer_view);

    // delayed injection
    void setDevice(VkDevice device);

    VkResult create(const VkBufferViewCreateInfo& createInfo);

    VkResult create(VkBuffer buffer, VkFormat format, VkDeviceSize offset = 0, VkDeviceSize range = 0 /*, VkBufferViewCreateFlags flags*/);
};

}
