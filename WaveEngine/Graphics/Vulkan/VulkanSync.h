#pragma once
#include "VulkanCommonHeaders.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

struct deviceContext;

// Host-Device Sync
class vulkanFence {
private:
    VkFence                     _fence{ VK_NULL_HANDLE };
    VkDevice                    _device{ VK_NULL_HANDLE };

public:
    vulkanFence() = default;

    DISABLE_COPY(vulkanFence)

    vulkanFence(const deviceContext& dCtx, VkFenceCreateFlags flags = 0, const void* next = nullptr) {
        create(dCtx, flags, next);
    }

    VK_MOVE_CTOR2(vulkanFence, _fence, _device)
    VK_MOVE_ASSIGN2(vulkanFence, _fence, _device)

    ~vulkanFence() {
        destroy();
    }

    [[nodiscard]] VK_DEFINE_PTR_TYPE_OPERATOR(_fence)
    [[nodiscard]] VK_DEFINE_ADDRESS_FUNCTION(_fence)

    [[nodiscard]] constexpr VkFence fence() const {
        assert(_fence != VK_NULL_HANDLE);
        return _fence;
    }

    VkResult wait() const;
    VkResult wait(u64 timeout) const;
    VkResult wait(VkBool32 waitAll, u64 timeout) const;
    VkResult reset();
    VkResult waitAndReset();

    // VK_SUCCESS   -> signaled
    // VK_NOT_READY -> unsignaled
    // < 0          -> error
    VkResult status() const;

    VkResult create(const deviceContext& dCtx, VkFenceCreateFlags flags = 0, const void* next = nullptr);
    void destroy() noexcept;
};

// Between-Queues Sync / In-Queue Sync
class vulkanSemaphore {
private:
    VkSemaphore                 _semaphore{ VK_NULL_HANDLE };
    VkDevice                    _device{ VK_NULL_HANDLE };

public:
    vulkanSemaphore() = default;

    DISABLE_COPY(vulkanSemaphore)

    vulkanSemaphore(const deviceContext& dCtx, VkSemaphoreCreateFlags flags = 0, const void* next = nullptr) {
        create(dCtx, flags, next);
    }

    VK_MOVE_CTOR2(vulkanSemaphore, _semaphore, _device)
    VK_MOVE_ASSIGN2(vulkanSemaphore, _semaphore, _device)

    ~vulkanSemaphore() {
        destroy();
    }

    [[nodiscard]] VK_DEFINE_PTR_TYPE_OPERATOR(_semaphore)
    [[nodiscard]] VK_DEFINE_ADDRESS_FUNCTION(_semaphore)

    [[nodiscard]] constexpr VkSemaphore semaphore() const {
        assert(_semaphore != VK_NULL_HANDLE);
        return _semaphore;
    }

    // TODO maybe not useful
    VkResult wait(VkSemaphoreWaitInfo& waitInfo) const;

    VkResult create(const deviceContext& dCtx, VkSemaphoreCreateFlags flags = 0, const void* next = nullptr);
    void destroy() noexcept;
};

// Pipeline barrier
// In-Queue Sync
// call vkCmdPipelineBarrier(), executed by a command buffer

// Execution barrier / Memory barrier

// Host-Device Sync / Between-Commands in single Queue
// GPU has to wait _event to be Signaled to proceed, while CPU doesn't have to
// CPU can query event status
class vulkanEvent {
private:
    VkEvent                     _event{ VK_NULL_HANDLE };
    VkDevice                    _device{ VK_NULL_HANDLE };

public:
    vulkanEvent() = default;

    DISABLE_COPY(vulkanEvent);

    vulkanEvent(const deviceContext& dCtx, VkEventCreateFlags flags = 0, const void* next = nullptr) {
        create(dCtx, flags, next);
    }

    VK_MOVE_CTOR2(vulkanEvent, _event, _device)
    VK_MOVE_ASSIGN2(vulkanEvent, _event, _device)

    ~vulkanEvent() {
        VK_DESTROY_PTR_BY(vkDestroyEvent, _device, _event)
    }

    [[nodiscard]] VK_DEFINE_PTR_TYPE_OPERATOR(_event)
    [[nodiscard]] VK_DEFINE_ADDRESS_FUNCTION(_event)

    [[nodiscard]] constexpr VkEvent event() const {
        assert(_event != VK_NULL_HANDLE);
        return _event;
    }

    void cmdSet(VkCommandBuffer commandBuffer, VkPipelineStageFlags stageMask) const;
    void cmdReset(VkCommandBuffer commandBuffer, VkPipelineStageFlags stageMask) const;

    void cmdWait(VkCommandBuffer cmd, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                            uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers,
                            uint32_t bufferBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers,
                            uint32_t imageBarrierCount, const VkImageMemoryBarrier* pImageBarriers) const;

    // set sType outside
    void cmdWait(VkCommandBuffer cmdBuffer,
                VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                const UTL::vector<VkMemoryBarrier>& memoryBarriers,
                const UTL::vector<VkBufferMemoryBarrier>& bufferMemoryBarriers,
                const UTL::vector<VkImageMemoryBarrier>& imageBarriers) const;

    VkResult set() const;
    VkResult reset() const;

    // VK_EVENT_SET     -> set
    // VK_EVENT_RESET   -> unset
    // < 0              -> error
    VkResult status() const;

    VkResult create(const deviceContext& dCtx, VkEventCreateFlags flags = 0, const void* next = nullptr);
};

}
