#pragma once
#include "VulkanCommonHeaders.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

// Host-Device Sync
class vulkanFence {
private:
    VkFence                     _fence{ VK_NULL_HANDLE };
    VkDevice                    _device{ VK_NULL_HANDLE };

public:
    vulkanFence(VkDevice device) : _device(device) {}
    vulkanFence(VkDevice device, VkFenceCreateInfo& createInfo, VkFenceCreateFlags flags = 0) : _device(device) {
        create(createInfo, flags);
    }

    vulkanFence(vulkanFence&& other) noexcept {
        VK_MOVE_PTR(_fence);
        VK_MOVE_PTR(_device);
    }
    ~vulkanFence() {
        VK_DESTROY_PTR_BY(vkDestroyFence, _device, _fence);
    }

    VK_DEFINE_PTR_TYPE_OPERATOR(_fence);
    VK_DEFINE_ADDRESS_FUNCTION(_fence);

    VkResult wait() const;
    VkResult reset() const;
    VkResult waitAndReset() const;

    VkResult status() const;

    VkResult create(VkFenceCreateInfo& createInfo, VkFenceCreateFlags flags = 0);
};

// Between-Queues Sync / In-Queue Sync
class vulkanSemaphore {
private:
    VkSemaphore                 _semaphore{ VK_NULL_HANDLE };
    VkDevice                    _device{ VK_NULL_HANDLE };

public:
    vulkanSemaphore(VkDevice device) : _device(device) {}
    vulkanSemaphore(VkDevice device, VkSemaphoreCreateInfo& createInfo, VkSemaphoreCreateFlags flags = 0) : _device(device) {
        create(createInfo, flags);
    }
    vulkanSemaphore(vulkanSemaphore&& other) noexcept {
        VK_MOVE_PTR(_semaphore);
        VK_MOVE_PTR(_device);
    }
    ~vulkanSemaphore() {
        VK_DESTROY_PTR_BY(vkDestroySemaphore, _device, _semaphore);
    }

    VK_DEFINE_PTR_TYPE_OPERATOR(_semaphore);
    VK_DEFINE_ADDRESS_FUNCTION(_semaphore);

    // TODO
    VkResult wait(VkSemaphoreWaitInfo& waitInfo) const;

    VkResult create(VkSemaphoreCreateInfo& createInfo, VkSemaphoreCreateFlags flags = 0);
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
    vulkanEvent(VkDevice device) : _device(device) {}
    vulkanEvent(VkDevice device, VkEventCreateInfo& createInfo, VkEventCreateFlags flags = 0) : _device(device) {
        create(createInfo, flags);
    }
    vulkanEvent(vulkanEvent&& other) noexcept {
        VK_MOVE_PTR(_event);
        VK_MOVE_PTR(_device);
    }
    ~vulkanEvent() {
        VK_DESTROY_PTR_BY(vkDestroyEvent, _device, _event);
    }

    VK_DEFINE_PTR_TYPE_OPERATOR(_event);
    VK_DEFINE_ADDRESS_FUNCTION(_event);

    void cmdSet(VkCommandBuffer commandBuffer, VkPipelineStageFlags stageFrom) const;
    void cmdReset(VkCommandBuffer commandBuffer, VkPipelineStageFlags stageFrom) const;
    void cmdWait(VkCommandBuffer commandBuffer, VkPipelineStageFlags stageFrom, VkPipelineStageFlags stageTo,
                UTL::vector<VkMemoryBarrier> memoryBarriers, UTL::vector<VkBufferMemoryBarrier> bufferMemoryBarriers,
                UTL::vector<VkImageMemoryBarrier> imageMemoryBarriers) const;

    VkResult set() const;
    VkResult reset() const;
    VkResult status() const;

    VkResult create(VkEventCreateInfo& createInfo, VkEventCreateFlags flags = 0);
};

}
