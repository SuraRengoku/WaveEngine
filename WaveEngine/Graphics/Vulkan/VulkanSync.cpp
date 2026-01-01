#include "VulkanSync.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

VkResult vulkanFence::wait() const {
    if (VkResult result = vkWaitForFences(_device, 1, &_fence, false, UINT64_MAX)) {
        debug_error("::VULKAN:ERROR [ fence ] Failed to wait for the fence\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanFence::reset() const {
    if (VkResult result = vkResetFences(_device, 1, &_fence)) {
        debug_error("::VULKAN:ERROR [ fence ] Failed to reset the fence\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanFence::waitAndReset() const {
    VkResult result = wait();
    result || (result = reset());
    return result;
}

VkResult vulkanFence::status() const {
    VkResult result = vkGetFenceStatus(_device, _fence);
    if (result < 0) {
        debug_error("::VULKAN:ERROR Failed to get the status of fence\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanFence::create(VkFenceCreateInfo& createInfo, VkFenceCreateFlags flags) {
    assert(_device);
    createInfo.flags = flags;
    if (VkResult result = vkCreateFence(_device, &createInfo, nullptr, &_fence)) {
        debug_error("::VULKAN:ERROR Failed to create a fence\n");
        return result;
    }
    return VK_SUCCESS;
}

// TODO
VkResult vulkanSemaphore::wait(VkSemaphoreWaitInfo& waitInfo) const {
    waitInfo.pSemaphores = &_semaphore;
    if (VkResult result = vkWaitSemaphores(_device, &waitInfo, UINT64_MAX)) {
        debug_error("::VULKAN:ERROR Failed to wait for the semaphore\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanSemaphore::create(VkSemaphoreCreateInfo& createInfo, VkSemaphoreCreateFlags flags) {
    assert(_device != VK_NULL_HANDLE);
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    createInfo.flags = flags;
    if (VkResult result = vkCreateSemaphore(_device, &createInfo, nullptr, &_semaphore)) {
        debug_error("::VULKAN:ERROR Failed to create a semaphore\n");
        return result;
    }
    return VK_SUCCESS;
}

void vulkanEvent::cmdSet(VkCommandBuffer commandBuffer, VkPipelineStageFlags stageFrom) const {
    vkCmdSetEvent(commandBuffer, _event, stageFrom);
}

void vulkanEvent::cmdReset(VkCommandBuffer commandBuffer, VkPipelineStageFlags stageFrom) const {
    vkCmdResetEvent(commandBuffer, _event, stageFrom);
}

void vulkanEvent::cmdWait(VkCommandBuffer commandBuffer, VkPipelineStageFlags stageFrom, VkPipelineStageFlags stageTo,
    UTL::vector<VkMemoryBarrier> memoryBarriers, UTL::vector<VkBufferMemoryBarrier> bufferMemoryBarriers,
    UTL::vector<VkImageMemoryBarrier> imageMemoryBarriers) const {
    for (auto& memoryBarrier : memoryBarriers)
        memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    for (auto& bufferMemoryBarrier : bufferMemoryBarriers)
        bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    for (auto& imageMemoryBarrier : imageMemoryBarriers)
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    vkCmdWaitEvents(commandBuffer, 1, &_event, stageFrom, stageTo,
        memoryBarriers.size(), memoryBarriers.data(),
        bufferMemoryBarriers.size(), bufferMemoryBarriers.data(),
        imageMemoryBarriers.size(), imageMemoryBarriers.data());
}

VkResult vulkanEvent::set() const {
    // CPU set event to signaled
    if (VkResult result = vkSetEvent(_device, _event)) {
        debug_error("::VULKAN:ERROR Failed to set event\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanEvent::reset() const {
    // CPU set event to unsignaled
    if (VkResult result = vkResetEvent(_device, _event)) {
        debug_error("::VULKAN:ERROR Failed to reset event\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanEvent::status() const {
    if (VkResult result = vkGetEventStatus(_device, _event)) {
        debug_error("::VULKAN:ERROR Failed to query the event status\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanEvent::create(VkEventCreateInfo& createInfo, VkEventCreateFlags flags) {
    assert(_device != VK_NULL_HANDLE);
    createInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    createInfo.flags = flags;
    if (VkResult result = vkCreateEvent(_device, &createInfo, nullptr, &_event)) {
        debug_error("::VULKAN:ERROR Failed to create an event\n");
        return result;
    }
    return VK_SUCCESS;
}
}
