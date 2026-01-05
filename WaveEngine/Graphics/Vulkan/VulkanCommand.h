#pragma once
#include "VulkanCommonHeaders.h"
#include "VulkanContext.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

class vulkanCommandBuffer;

// for better performance, we should allocate different command pools to each thread to avoid accessing conflicts
class vulkanCommandPool {
private:
    VkCommandPool                       _pool{ VK_NULL_HANDLE };
    VkDevice                            _device{ VK_NULL_HANDLE };

public:
    vulkanCommandPool() = default;
    vulkanCommandPool(vulkanCommandPool&& other) noexcept {
        VK_MOVE_PTR(_pool);
        VK_MOVE_PTR(_device);
    }
    ~vulkanCommandPool() {
        VK_DESTROY_PTR_BY(vkDestroyCommandPool, _device, _pool);
        assert(_pool == VK_NULL_HANDLE);
        _device = VK_NULL_HANDLE;
    }

    VK_DEFINE_PTR_TYPE_OPERATOR(_pool);
    VK_DEFINE_ADDRESS_FUNCTION(_pool);

    bool initialize(VkDevice device, u32 queueFamilyIndex, const void* next = nullptr, VkCommandPoolCreateFlags flags = 0);
    bool initialize(const deviceContext& dCtx, const void* next = nullptr, VkCommandPoolCreateFlags flags = 0);

    VkResult allocateBuffers(UTL::vector<VkCommandBuffer>& buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const;
    VkResult allocateBuffers(UTL::vector<vulkanCommandBuffer>& buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const;

    void freeBuffers(UTL::vector<VkCommandBuffer>& buffers) const;
    void freeBuffers(UTL::vector<vulkanCommandBuffer>& buffers) const;

    // no release
};

class vulkanCommandBuffer {
private:
    friend class vulkanCommandPool; // to let the pool access private field _commandBuffer
    VkCommandBuffer                     _commandBuffer{ VK_NULL_HANDLE };

public:
    vulkanCommandBuffer() = default;

    DISABLE_COPY(vulkanCommandBuffer);

    vulkanCommandBuffer(vulkanCommandBuffer&& other) noexcept {
        VK_MOVE_PTR(_commandBuffer);
    }
    // no destructor because the function releasing CommandBuffer has been defined in vulkanCommandPool
    VK_DEFINE_PTR_TYPE_OPERATOR(_commandBuffer);
    VK_DEFINE_ADDRESS_FUNCTION(_commandBuffer);

    // begin recording
    VkResult begin(VkCommandBufferUsageFlags usageFlags, const VkCommandBufferInheritanceInfo& inheritanceInfo) const;
    VkResult begin(VkCommandBufferUsageFlags usageFlags = 0) const;

    // finish recording
    VkResult end() const;
};

}
