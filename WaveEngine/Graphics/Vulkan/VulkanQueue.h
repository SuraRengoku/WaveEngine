#pragma once
#include "VulkanCommonHeaders.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

struct frameContext;

class vulkanQueue {
public:
    vulkanQueue() = default;
    void initialize(VkDevice device, u32 familyIndex, u32 queueIndex = 0);

    VkResult submit(const VkSubmitInfo* submitInfo, VkFence fence = VK_NULL_HANDLE) const;
    VkResult submit(const frameContext& frameCtx, const VkPipelineStageFlags* waitStages, const void* next = nullptr) const;

    VkResult present(const VkPresentInfoKHR* presentInfo) const;
    VkResult present(const frameContext& frameCtx, const VkSwapchainKHR& swapchain, const u32& imageIdx, VkResult* results, const void* next = nullptr) const;

    VkResult waitIdle() const;

    [[nodiscard]] VkQueue queue() const { return _queue; }
    [[nodiscard]] u32 familyIndex() const { return _familyIndex; }

private:
    VkQueue                 _queue{ VK_NULL_HANDLE };
    u32                     _familyIndex{ 0 };
    VkDevice                _device{ VK_NULL_HANDLE };
    mutable std::mutex      _mutex;
};

}
