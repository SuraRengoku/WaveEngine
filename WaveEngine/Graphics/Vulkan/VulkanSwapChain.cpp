#include "VulkanSwapChain.h"
#include "VulkanHelpers.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

DETAIL::SwapChainSupportDetails vulkanSwapChain::querySwapChainSupport() const {
    DETAIL::SwapChainSupportDetails details{};
    assert(_physical_device);
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physical_device, _surface, &details.capabilities);

    u32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device, _surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device, _surface, &formatCount, details.formats.data());
    }

    u32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

void vulkanSwapChain::chooseSwapSurfaceFormat(const UTL::vector<VkSurfaceFormatKHR>& surfaceFormats) {
    if (surfaceFormats.size() == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED) {
        _surface_format = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
        return;
    }
    for (const auto& availableFormat : surfaceFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            _surface_format = availableFormat;
            return;
        }
    }
    // we can still score each available format to choose the best one
    // however, sometimes the first one is no matter a good choice
    _surface_format = surfaceFormats[0];
}

void vulkanSwapChain::chooseSwapPresentMode(const UTL::vector<VkPresentModeKHR>& presentModes) {
    VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& availablePresentMode : presentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR || availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            bestMode = availablePresentMode;
        }
    }
    _present_mode = bestMode;
}

void vulkanSwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
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
        // TODO get window handle in MacOS / IOS
#elif defined(__linux__)
        // TODO get window handle in wayland / x11
#elif defined(__ANDROID__)
        // TODO get window handle in android
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
    DETAIL::SwapChainSupportDetails details = querySwapChainSupport();
    chooseSwapSurfaceFormat(details.formats);
    chooseSwapPresentMode(details.presentModes);
    chooseSwapExtent(details.capabilities);

    // in order to support tri-buffer, let the chain support one more graphics than the minimum
    u32 imageCount = details.capabilities.minImageCount + 1;
    if (details.capabilities.maxImageCount > 0
        && imageCount > details.capabilities.maxImageCount) {
        imageCount = details.capabilities.maxImageCount; // limit the actual image count
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
    createInfo.imageArrayLayers = 1; // #view point(s), for VR this variable maybe more than 1
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

    createInfo.preTransform = details.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // alpha channel should be used to blend between surface window and other windows
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE; // TODO if we have old swapchain

    VKCall(vkCreateSwapchainKHR(_device, &createInfo, nullptr, &_swap_chain), "::VULKAN:ERROR Failed to create a swap chain\n");

    vkGetSwapchainImagesKHR(_device, _swap_chain, &imageCount, nullptr);
    _images.resize(imageCount);
    vkGetSwapchainImagesKHR(_device, _swap_chain, &imageCount, _images.data());

    createImageViews();

#ifdef _DEBUG
    debug_output("::VULKAN:INFO Swapchain successfully created\n");
#endif
}

void vulkanSwapChain::createImageViews() {
    _image_views.clear();
    _image_views.resize(_images.size());

    VkImageSubresourceRange range{};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    deviceContext dCtx{_device, {}, {}, nullptr};
    VkComponentMapping defaultComponents{};

    for (u32 i{0}; i < _images.size(); ++i) {
        assert(!_image_views[i].isValid());
        _image_views[i].create(dCtx, _images[i], _surface_format.format, VK_IMAGE_VIEW_TYPE_2D, defaultComponents, range);
    }
}

void vulkanSwapChain::recreate() {
    create();
    // TODO recreate related resources
    // create Render Pass
    // create Graphics Pipeline
    // create Color Resources
    // create Depth Resources
    // create Frame Buffers
    // create Command Buffers
}

void vulkanSwapChain::release() {
    vkDeviceWaitIdle(_device);

    // TODO before clean swapchain we should clean all resources
    _image_views.clear();
    if (_swap_chain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(_device, _swap_chain, nullptr);
        _swap_chain = VK_NULL_HANDLE;
    }
}

}
