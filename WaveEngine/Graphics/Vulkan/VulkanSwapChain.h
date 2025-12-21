#pragma once
#include <vulkan/vulkan_core.h>

namespace WAVEENGINE::GRAPHICS::VULKAN {

class vulkanSwapChain {

private:
    VkSwapchainKHR              _swap_chain {VK_NULL_HANDLE};
};

}
