#include "VulkanSync.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

VkResult vulkanFence::wait() {
    return wait(UINT64_MAX);
}

VkResult vulkanFence::wait(u64 timeout) {
    if (VkResult result = vkWaitForFences(_device, 1, &_fence, false, timeout)) {
        debug_error("::VULKAN:ERROR [ fence ] Failed to wait for the fence\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanFence::reset() {
    if (VkResult result = vkResetFences(_device, 1, &_fence)) {
        debug_error("::VULKAN:ERROR [ fence ] Failed to reset the fence\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanFence::waitAndReset() {
    VkResult result = wait();
    result || (result = reset());
    return result;
}

VkResult vulkanFence::status() const {
    assert(_fence != VK_NULL_HANDLE);
    return vkGetFenceStatus(_device, _fence);
}

VkResult vulkanFence::create(const deviceContext& dCtx, VkFenceCreateFlags flags, const void* next) {
    assert(_fence == VK_NULL_HANDLE && "::VULKAN:ERROR Can not recreate fence\n");
    _device = dCtx._device;

    VkFenceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    createInfo.flags = flags;
    createInfo.pNext = next;

    if (VkResult result = vkCreateFence(_device, &createInfo, dCtx._allocator, &_fence)) {
        debug_error("::VULKAN:ERROR Failed to create a fence\n");
        return result;
    }
    return VK_SUCCESS;
}

// TODO maybe not useful
VkResult vulkanSemaphore::wait(VkSemaphoreWaitInfo& waitInfo) const {
    waitInfo.pSemaphores = &_semaphore;
    if (VkResult result = vkWaitSemaphores(_device, &waitInfo, UINT64_MAX)) {
        debug_error("::VULKAN:ERROR Failed to wait for the semaphore\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanSemaphore::create(const deviceContext& dCtx, VkSemaphoreCreateFlags flags, const void* next) {
    assert(_semaphore == VK_NULL_HANDLE && "::VULKAN:ERROR Can not recreate semaphore\n");
    _device = dCtx._device;

    VkSemaphoreCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    createInfo.flags = flags;
    createInfo.pNext = next;

    if (VkResult result = vkCreateSemaphore(_device, &createInfo, dCtx._allocator, &_semaphore)) {
        debug_error("::VULKAN:ERROR Failed to create a semaphore\n");
        return result;
    }
    return VK_SUCCESS;
}

void vulkanEvent::cmdSet(VkCommandBuffer commandBuffer, VkPipelineStageFlags stageMask) const {
    vkCmdSetEvent(commandBuffer, _event, stageMask);
}

void vulkanEvent::cmdReset(VkCommandBuffer commandBuffer, VkPipelineStageFlags stageMask) const {
    vkCmdResetEvent(commandBuffer, _event, stageMask);
}

void vulkanEvent::cmdWait(VkCommandBuffer cmd, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
    uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferBarrierCount,
    const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageBarrierCount,
    const VkImageMemoryBarrier* pImageBarriers) const {
    vkCmdWaitEvents(cmd, 1, &_event, srcStage, dstStage,
                    memoryBarrierCount, pMemoryBarriers,
    bufferBarrierCount, pBufferMemoryBarriers,
    imageBarrierCount, pImageBarriers);
}

void vulkanEvent::cmdWait(VkCommandBuffer cmdBuffer, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                          const UTL::vector<VkMemoryBarrier>& memoryBarriers,
                          const UTL::vector<VkBufferMemoryBarrier>& bufferMemoryBarriers,
                          const UTL::vector<VkImageMemoryBarrier>& imageMemoryBarriers) const {
    vkCmdWaitEvents(cmdBuffer, 1, &_event, srcStage, dstStage,
        static_cast<u32>(memoryBarriers.size()), memoryBarriers.data(),
        static_cast<u32>(bufferMemoryBarriers.size()), bufferMemoryBarriers.data(),
        static_cast<u32>(imageMemoryBarriers.size()), imageMemoryBarriers.data());
}

VkResult vulkanEvent::set() const {
    // Host set event to signaled
    if (VkResult result = vkSetEvent(_device, _event)) {
        debug_error("::VULKAN:ERROR Failed to set event\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanEvent::reset() const {
    // Host set event to unsignaled
    if (VkResult result = vkResetEvent(_device, _event)) {
        debug_error("::VULKAN:ERROR Failed to reset event\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanEvent::status() const {
    assert(_event != VK_NULL_HANDLE);
    return vkGetEventStatus(_device, _event);
}

VkResult vulkanEvent::create(const deviceContext& dCtx, VkEventCreateFlags flags, const void* next) {
    assert(_event == VK_NULL_HANDLE && "::VULKAN:ERROR Can not recreate event\n");
    _device = dCtx._device;

    VkEventCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    createInfo.flags = flags;
    createInfo.pNext = next;

    if (VkResult result = vkCreateEvent(_device, &createInfo, nullptr, &_event)) {
        debug_error("::VULKAN:ERROR Failed to create an event\n");
        return result;
    }
    return VK_SUCCESS;
}
}
