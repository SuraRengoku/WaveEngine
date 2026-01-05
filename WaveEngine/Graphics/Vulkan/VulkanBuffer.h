#pragma once
#include "VulkanCommonHeaders.h"
#include "VulkanContext.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

class vulkanBuffer {
private:
    VkBuffer                _buffer{ VK_NULL_HANDLE };
    VkDevice                _device{ VK_NULL_HANDLE };

public:
    vulkanBuffer() = default;

    DISABLE_COPY(vulkanBuffer);

    vulkanBuffer(const deviceContext& dCtx, const VkBufferCreateInfo& createInfo) {
        create(dCtx, createInfo);
    }
    vulkanBuffer(const deviceContext& dCtx, VkSharingMode sMode, VkDeviceSize dSize,
                            const VkBufferCreateFlags cFlags = 0, const VkBufferUsageFlags uFlags = 0,
                            const void* next = nullptr) {
        create(dCtx, sMode, dSize, cFlags, uFlags, next);
    }

    VK_MOVE_CTOR2(vulkanBuffer, _buffer, _device);
    VK_MOVE_ASSIGN2(vulkanBuffer, _buffer, _device);

    ~vulkanBuffer() {
        VK_DESTROY_PTR_BY(vkDestroyBuffer, _device, _buffer);
        assert(_buffer == VK_NULL_HANDLE);
        _device = VK_NULL_HANDLE;
    }

    [[nodiscard]] VK_DEFINE_PTR_TYPE_OPERATOR(_buffer);
    [[nodiscard]] VK_DEFINE_ADDRESS_FUNCTION(_buffer);

    [[nodiscard]] VkBuffer getBuffer() const {
        assert(_buffer != VK_NULL_HANDLE);
        return _buffer;
    }

    bool create(const deviceContext& dCtx, const VkBufferCreateInfo& createInfo);
    bool create(const deviceContext& dCtx, VkSharingMode sMode, VkDeviceSize dSize,
                            const VkBufferCreateFlags cFlags = 0, const VkBufferUsageFlags uFlags = 0,
                            const void* next = nullptr);

    bool isValid() const noexcept {
        return _buffer != VK_NULL_HANDLE;
    }

    VkMemoryAllocateInfo memoryAllocateInfo(VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags desiredMemoryProperties) const;

    VkResult bindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset = 0);
};

class vulkanBufferView {
private:
    VkBufferView            _buffer_view{ VK_NULL_HANDLE };
    VkDevice                _device{ VK_NULL_HANDLE };

public:
    vulkanBufferView() = default;

    DISABLE_COPY(vulkanBufferView);

    vulkanBufferView(const deviceContext& dCtx, const VkBufferViewCreateInfo& createInfo) {
        create(dCtx, createInfo);
    }

    vulkanBufferView(const deviceContext& dCtx, VkBuffer buffer, VkFormat format, VkDeviceSize offset = 0,
                    const VkDeviceSize range = 0, const VkBufferViewCreateFlags flags = 0, const void* next = nullptr) {
        create(dCtx, buffer, format, offset, range, flags, next);
    }

    VK_MOVE_CTOR2(vulkanBufferView, _buffer_view, _device);
    VK_MOVE_ASSIGN2(vulkanBufferView, _buffer_view, _device);

    ~vulkanBufferView() {
        VK_DESTROY_PTR_BY(vkDestroyBufferView, _device, _buffer_view);
        assert(_buffer_view == VK_NULL_HANDLE);
        _device = VK_NULL_HANDLE;
    }

    [[nodiscard]] VK_DEFINE_PTR_TYPE_OPERATOR(_buffer_view);
    [[nodiscard]] VK_DEFINE_ADDRESS_FUNCTION(_buffer_view);

    [[nodiscard]] VkBufferView getBufferView() const {
        assert(_buffer_view != VK_NULL_HANDLE);
        return _buffer_view;
    }

    bool create(const deviceContext& dCtx, const VkBufferViewCreateInfo& createInfo);
    bool create(const deviceContext& dCtx, VkBuffer buffer, VkFormat format, VkDeviceSize offset = 0,
                    const VkDeviceSize range = 0, const VkBufferViewCreateFlags flags = 0, const void* next = nullptr);

    bool isValid() const noexcept {
        return _buffer_view != VK_NULL_HANDLE;
    }
};

}
