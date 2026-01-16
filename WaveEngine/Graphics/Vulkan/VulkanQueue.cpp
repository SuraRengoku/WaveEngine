#include "VulkanQueue.h"
#include "VulkanContext.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

void vulkanQueue::initialize(VkDevice device, u32 familyIndex, u32 queueIndex) {
    _device = device;
    _familyIndex = familyIndex;
    vkGetDeviceQueue(device, familyIndex, queueIndex, &_queue);
}

VkResult vulkanQueue::submit(const VkSubmitInfo* submitInfo, VkFence fence) const{
    std::lock_guard lock{ _mutex };
    return vkQueueSubmit(_queue, 1, submitInfo, fence);
}

VkResult vulkanQueue::submit(const frameContext& frameCtx, const VkPipelineStageFlags* waitStages, const void* next) const {
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = next;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = frameCtx.image_available_semaphore.Address();
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = frameCtx.graphics_cmd_buffer.Address();
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = frameCtx.render_finished_semaphore.Address();

    return submit(&submitInfo, frameCtx.fence);
}

VkResult vulkanQueue::present(const VkPresentInfoKHR* presentInfo) const {
    std::lock_guard lock{ _mutex };
    return vkQueuePresentKHR(_queue, presentInfo);
}

VkResult vulkanQueue::present(const frameContext& frameCtx, const VkSwapchainKHR& swapchain, const u32& imageIdx, VkResult* results, const void* next) const {
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = next;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = frameCtx.render_finished_semaphore.Address();
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIdx;
    presentInfo.pResults = results;

    return present(&presentInfo);
}

VkResult vulkanQueue::waitIdle() const {
    std::lock_guard lock{ _mutex };
    return vkQueueWaitIdle(_queue);
}

}
