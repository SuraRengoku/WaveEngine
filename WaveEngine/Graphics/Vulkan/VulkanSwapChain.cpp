#include "VulkanSwapChain.h"
#include "VulkanHelpers.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

void vulkanSwapChain::querySwapChainSupport() {
    assert(_physical_device);
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physical_device, _surface, &_chain_support_details.capabilities);

    u32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device, _surface, &formatCount, nullptr);
    if (formatCount != 0) {
        _chain_support_details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device, _surface, &formatCount, _chain_support_details.formats.data());
    }

    u32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        _chain_support_details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface, &presentModeCount, _chain_support_details.presentModes.data());
    }
}

void vulkanSwapChain::chooseSwapSurfaceFormat() {
    const auto& availableFormats = _chain_support_details.formats;
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
        _surface_format = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
        return;
    }
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            _surface_format = availableFormat;
            return;
        }
    }
    // we can still score each available format to choose the best one
    // however, sometimes the first one is no matter a good choice
    _surface_format = availableFormats[0];
}

void vulkanSwapChain::chooseSwapPresentMode() {
    const auto& availablePresentModes = _chain_support_details.presentModes;
    VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR || availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            bestMode = availablePresentMode;
        }
    }
    _present_mode = bestMode;
}

void vulkanSwapChain::chooseSwapExtent() {
    const auto& capabilities = _chain_support_details.capabilities;
    if (capabilities.currentExtent.width != std::numeric_limits<u32>::max()) {
        _extent = capabilities.currentExtent;
    } else {
        u32 width, height;

#if _WIN32
        HWND hwnd{ static_cast<HWND>(_window.handle()) };
        RECT rect;
        GetClientRect(hwnd, &rect);
        width = rect.right - rect.left;
        height = rect.bottom - rect.top;
#elif defined(__APPLE__)
        // TODO
#elif defined(__linux__)
        // TODO
#elif defined(__ANDROID__)
        // TODO
#endif

        VkExtent2D actualExtent = {width, height};
        actualExtent.width = std::max(capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height,
    std::       min(capabilities.maxImageExtent.height, actualExtent.height));
        _extent = actualExtent;
    }
}

void vulkanSwapChain::create() {
    // in order to support tri-buffer, let the chain support one more graphics than the minimum
    u32 imageCount = _chain_support_details.capabilities.minImageCount + 1;
    if (_chain_support_details.capabilities.maxImageCount > 0
        && imageCount > _chain_support_details.capabilities.maxImageCount) {
        imageCount = _chain_support_details.capabilities.maxImageCount; // limit the actual image count
    }
    // we can use any number of graphics if the memory can fulfill when the maxImageCount is 0

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = _surface_format.format;
    createInfo.imageColorSpace = _surface_format.colorSpace;
    createInfo.presentMode = _present_mode;
    createInfo.imageExtent = _extent;
    createInfo.imageArrayLayers = 1; // for VR this variable maybe more than 1
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VKX::QueueFamilyIndices indices = VKX::findQueueFamilies(_physical_device, _surface);
    u32 queueFamilyIndices[] = { static_cast<u32>(indices.graphicsFamily), static_cast<u32>(indices.presentFamily) };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = _chain_support_details.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // alpha channel should be used to blend between surface window and other windows
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE; // TODO

    VKCall(vkCreateSwapchainKHR(_device, &createInfo, nullptr, &_swap_chain), "::VULKAN: failed to create swap chain\n");
}

void vulkanSwapChain::release() {
    vkDeviceWaitIdle(_device);

    // TODO

    vkDestroySwapchainKHR(_device, _swap_chain, nullptr);
}
}
