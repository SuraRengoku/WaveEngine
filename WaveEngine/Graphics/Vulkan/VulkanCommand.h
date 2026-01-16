#pragma once
#include "VulkanCommonHeaders.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

struct deviceContext;

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
        assert(_pool == VK_NULL_HANDLE);
        _device = VK_NULL_HANDLE;
    }

    [[nodiscard]] VK_DEFINE_PTR_TYPE_OPERATOR(_pool);
    [[nodiscard]] VK_DEFINE_ADDRESS_FUNCTION(_pool);

    bool initialize(VkDevice device, u32 queueFamilyIndex, const void* next = nullptr, VkCommandPoolCreateFlags flags = 0);
    bool initialize(const deviceContext& dCtx, const void* next = nullptr, VkCommandPoolCreateFlags flags = 0);

    VkResult allocateBuffers(UTL::vector<VkCommandBuffer>& buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const;
    VkResult allocateBuffers(UTL::vector<vulkanCommandBuffer>& buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const;

    void freeBuffers(UTL::vector<VkCommandBuffer>& buffers) const;
    void freeBuffers(UTL::vector<vulkanCommandBuffer>& buffers) const;

    void release();
};

class vulkanCommandBuffer {
private:
    friend class vulkanCommandPool; // to let the pool access private field _commandBuffer
#if _DEBUG
    // vulkanCommandPool*                  container{ nullptr };
#endif
    VkCommandBuffer                     _commandBuffer{ VK_NULL_HANDLE };

public:
    vulkanCommandBuffer() = default;

    DISABLE_COPY(vulkanCommandBuffer);

    VK_MOVE_CTOR1(vulkanCommandBuffer, _commandBuffer);
    VK_MOVE_ASSIGN1(vulkanCommandBuffer, _commandBuffer);

    // no destructor because the function releasing CommandBuffer has been defined in vulkanCommandPool
    VK_DEFINE_PTR_TYPE_OPERATOR(_commandBuffer);
    VK_DEFINE_ADDRESS_FUNCTION(_commandBuffer);

    // begin recording
    VkResult begin(VkCommandBufferUsageFlags usageFlags, const VkCommandBufferInheritanceInfo& inheritanceInfo) const;
    VkResult begin(VkCommandBufferUsageFlags usageFlags = 0) const;

    void reset() const;

    // finish recording
    VkResult end() const;
};

}
