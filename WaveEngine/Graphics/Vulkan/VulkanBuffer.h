#pragma once
#include "VulkanCommonHeaders.h"
#include "VulkanCore.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

class vulkanBuffer {
private:
    VkBuffer                _buffer{ VK_NULL_HANDLE };

public:
    vulkanBuffer() = default;
    DISABLE_COPY(vulkanBuffer);
    vulkanBuffer(VkBufferCreateInfo& createInfo) {
        create(createInfo);
    }
    vulkanBuffer(vulkanBuffer&& other) noexcept {
        VK_MOVE_PTR(_buffer);
    }
    ~vulkanBuffer() {
        VK_DESTROY_PTR_BY(vkDestroyBuffer, CORE::device(), _buffer);
    }

    VK_DEFINE_PTR_TYPE_OPERATOR(_buffer);
    VK_DEFINE_ADDRESS_FUNCTION(_buffer);

    VkMemoryAllocateInfo memoryAllocateInfo(VkMemoryPropertyFlags desiredMemoryProperties) const;

    VkResult bindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset = 0) const;

    VkResult create(VkBufferCreateInfo& createInfo);
};

class vulkanBufferView {
private:
    VkBufferView            _buffer_view{ VK_NULL_HANDLE };
public:
    vulkanBufferView() = default;
    DISABLE_COPY(vulkanBufferView);
    vulkanBufferView(VkBufferViewCreateInfo& createInfo) {
        create(createInfo);
    }
    vulkanBufferView(VkBuffer buffer, VkFormat format, VkDeviceSize offset = 0, VkDeviceSize range = 0 /*, VkBufferViewCreateFlags flags*/) {
        create(buffer, format, offset, range);
    }
    vulkanBufferView(vulkanBufferView&& other) noexcept {
        VK_MOVE_PTR(_buffer_view);
    }
    ~vulkanBufferView() {
        VK_DESTROY_PTR_BY(vkDestroyBufferView, CORE::device(), _buffer_view);
    }

    VK_DEFINE_PTR_TYPE_OPERATOR(_buffer_view);
    VK_DEFINE_ADDRESS_FUNCTION(_buffer_view);

    VkResult create(VkBufferViewCreateInfo& createInfo);

    VkResult create(VkBuffer buffer, VkFormat format, VkDeviceSize offset = 0, VkDeviceSize range = 0 /*, VkBufferViewCreateFlags flags*/);
};

}
