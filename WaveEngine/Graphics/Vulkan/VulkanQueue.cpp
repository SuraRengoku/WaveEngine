#include "VulkanQueue.h"

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

VkResult vulkanQueue::present(const VkPresentInfoKHR* presentInfo) const {
    std::lock_guard lock{ _mutex };
    return vkQueuePresentKHR(_queue, presentInfo);
}

VkResult vulkanQueue::waitIdle() const {
    std::lock_guard lock{ _mutex };
    return vkQueueWaitIdle(_queue);
}

}
