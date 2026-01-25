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
    createInfo.oldSwapchain = VK_NULL_HANDLE; // TODO if we have old swap chain

    VKCall(vkCreateSwapchainKHR(_device, &createInfo, nullptr, &_swap_chain), "::VULKAN:ERROR Failed to create a swap chain\n");

    vkGetSwapchainImagesKHR(_device, _swap_chain, &imageCount, nullptr);
    _color_images.resize(imageCount);
    vkGetSwapchainImagesKHR(_device, _swap_chain, &imageCount, _color_images.data());

    createImageViews();

#ifdef _DEBUG
    debug_output("::VULKAN:INFO Swap chain successfully created\n");
    debug_output("  - Extent: %ux%u\n", _extent.width, _extent.height);
    debug_output("  - Format: %s\n", VKX::formatToString(_surface_format.format));
    debug_output("  - Color Space: %s\n", VKX::colorSpaceToString(_surface_format.colorSpace));
    debug_output("  - Present Mode: %s\n", VKX::presentModeToString(_present_mode));
    debug_output("  - Image Count: %u\n", static_cast<u32>(_color_images.size()));
#endif
}

void vulkanSwapChain::createImageViews() {
    _color_image_views.clear();
    _color_image_views.resize(_color_images.size());

    VkImageSubresourceRange range{};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    deviceContext dCtx{_device, {}, {}, nullptr};
    VkComponentMapping defaultComponents{};

    for (u32 i{0}; i < _color_images.size(); ++i) {
        assert(!_color_image_views[i].isValid());
        _color_image_views[i].create(dCtx, _color_images[i], _surface_format.format, VK_IMAGE_VIEW_TYPE_2D, defaultComponents, range);
    }
}

void vulkanSwapChain::createDepthResources() {
    VkFormat depthFormat = VKX::findDepthFormat(_physical_device);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = depthFormat;
    imageInfo.extent = { _extent.width, _extent.height, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VKCall(_depth_image.create(_device, _physical_device, imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
        "::VULKAN:ERROR Failed to create depth image\n");

    VkImageSubresourceRange range{};
    range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    deviceContext dCtx{ _device, {}, {}, nullptr };
    VkComponentMapping components{};
    _depth_image_view.create(dCtx, _depth_image.image(), depthFormat, VK_IMAGE_VIEW_TYPE_2D, components, range);

#ifdef _DEBUG
    debug_output("::VULKAN:INFO Swap chain depth resources successfully created\n");
    debug_output("  - Extent: %ux%ux%u\n", _extent.width, _extent.height, 1);
    debug_output("  - Format: %s\n", VKX::formatToString(depthFormat));
#endif
}

void vulkanSwapChain::destroyDepthResources() {
#ifdef _DEBUG
    if (_depth_image_view.isValid()) {
        debug_output("::VULKAN:INFO Destroying depth resources\n");
    }
#endif

    // TODO do we also have to destroy _depth_image
    _depth_image_view.destroy();
}

void vulkanSwapChain::createFramebuffers(const VkRenderPass& renderPass, bool withDepth) {
    destroyFramebuffers();

    if (withDepth) {
        createDepthResources();
    }

    _framebuffers.resize(_color_images.size());

    deviceContext dCtx{ _device, {}, {}, nullptr };

    for (u32 i{0}; i < _color_images.size(); ++i) {
	    UTL::vector<VkImageView> attachments = { _color_image_views[i].getImageView() };
        
        if (withDepth) {
            attachments.push_back(_depth_image_view);
        }

        VKCall(_framebuffers[i].create(dCtx, renderPass, static_cast<u32>(attachments.size()), attachments.data(), _extent.width, _extent.height, 1),
            "::VULKAN:ERROR Failed to create framebuffer\n");
    }

#ifdef _DEBUG
    debug_output("::VULKAN:INFO Swap chain framebuffers successfully created\n");
    debug_output("  - Framebuffer Count: %u\n", static_cast<u32>(_framebuffers.size()));
    debug_output("  - Dimensions: %ux%ux%u\n", _extent.width, _extent.height, 1);
    debug_output("  - Attachments per Framebuffer: %u (%s)\n",
        withDepth ? 2 : 1,
        withDepth ? "Color + Depth" : "Color only");
#endif
}

void vulkanSwapChain::destroyFramebuffers() {
#ifdef _DEBUG
    u32 count = static_cast<u32>(_framebuffers.size());
    if (count > 0) {
        debug_output("::VULKAN:INFO Destroying %u framebuffer(s)\n", count);
    }
#endif
    for (auto& framebuffer : _framebuffers) {
        framebuffer.destroy();
    }
    _framebuffers.clear();
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

    destroyFramebuffers();
    destroyDepthResources();

    // TODO before clean swap chain we should clean all resources
    _color_image_views.clear();
    if (_swap_chain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(_device, _swap_chain, nullptr);
        _swap_chain = VK_NULL_HANDLE;
    }
}

}
