#include "VulkanHelpers.h"

#include "VulkanBuffer.h"
#include "VulkanResources.h"

namespace  WAVEENGINE::GRAPHICS::VULKAN::VKX {

u32 rateDeviceSuitability(const VkPhysicalDevice& physical_device) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physical_device, &deviceProperties);

    // for texture compressing, multi viewport rendering
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(physical_device, &deviceFeatures);

    u32 score = 0;
    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }
    score += deviceProperties.limits.maxImageDimension2D;
    if (!deviceFeatures.geometryShader && !deviceFeatures.tessellationShader) {
        return 0;
    }
    return deviceFeatures.samplerAnisotropy == VK_TRUE ? score : 0;
}

VkSampleCountFlagBits getMaxUsableSampleCount(const VkPhysicalDevice& physical_device) {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physical_device, &physicalDeviceProperties);

    VkSampleCountFlags counts = std::min(
    physicalDeviceProperties.limits.framebufferColorSampleCounts,
    physicalDeviceProperties.limits.framebufferDepthSampleCounts);

    if (counts & VK_SAMPLE_COUNT_64_BIT)	return VK_SAMPLE_COUNT_64_BIT;
    if (counts & VK_SAMPLE_COUNT_32_BIT)	return VK_SAMPLE_COUNT_32_BIT;
    if (counts & VK_SAMPLE_COUNT_16_BIT)	return VK_SAMPLE_COUNT_16_BIT;
    if (counts & VK_SAMPLE_COUNT_8_BIT)		return VK_SAMPLE_COUNT_8_BIT;
    if (counts & VK_SAMPLE_COUNT_4_BIT)		return VK_SAMPLE_COUNT_4_BIT;
    if (counts & VK_SAMPLE_COUNT_2_BIT)		return VK_SAMPLE_COUNT_2_BIT;
    return VK_SAMPLE_COUNT_1_BIT;
}

QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& physical_device) {
    QueueFamilyIndices indices;
    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queueFamilyCount, nullptr);
    UTL::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queueFamilyCount, queueFamilies.data());

    for (u32 i{0}; i < queueFamilyCount; ++i) {
        const auto& queueFamily = queueFamilies[i];

        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = static_cast<u32>(i);
        }

        // Optional: find dedicated Compute queue
        if ((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) &&
            !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            indices.computeFamily = static_cast<u32>(i);
        }

        // Optional: find dedicated Transfer queue
        if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
            !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            !(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
            indices.transferFamily = static_cast<u32>(i);
        }

        if (indices.isComplete())
            break;
    }

    return indices;
}

QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& physical_device, const VkSurfaceKHR& surface) {
    QueueFamilyIndices indices;
    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queueFamilyCount, nullptr);
    UTL::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queueFamilyCount, queueFamilies.data());

    for (u32 i{0}; i < queueFamilyCount; ++i) {
        const auto& queueFamily = queueFamilies[i];

        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = static_cast<int>(i);
        }
        VkBool32 presentSupport = false;
        // check if the i-queue in device support the surface
        // which is important to make sure before setting swap chain
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = static_cast<int>(i);
        }

        // Optional: find dedicated queues
        if ((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) &&
            !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            indices.computeFamily = static_cast<u32>(i);
        }
        if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
            !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            !(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
            indices.transferFamily = static_cast<u32>(i);
        }

        if (indices.isComplete() && indices.hasPresentSupport()) {
            break;
        }
    }

    return indices;
}


VkPhysicalDeviceFeatures2 findPhysicalDeviceFeatures(const VkPhysicalDevice& physical_device) {
    VkPhysicalDeviceFeatures2 features{};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.pNext = nullptr;
    vkGetPhysicalDeviceFeatures2(physical_device, &features);
    return features;
}

VkPhysicalDeviceProperties2 findPhysicalDeviceProperties(const VkPhysicalDevice& physical_device) {
    VkPhysicalDeviceProperties2 properties;
    properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    properties.pNext = nullptr;
    vkGetPhysicalDeviceProperties2(physical_device, &properties);
    return properties;
}

VkPhysicalDeviceMemoryProperties2 findPhysicalDeviceMemoryProperties(const VkPhysicalDevice& physical_device) {
    VkPhysicalDeviceMemoryProperties2 memoryProperties;
    memoryProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
    memoryProperties.pNext = nullptr;
    vkGetPhysicalDeviceMemoryProperties2(physical_device, &memoryProperties);
    return memoryProperties;
}

VkFormat findDepthFormat(const VkPhysicalDevice& physical_device) {
    return findSupportedFormat(physical_device, { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat findSupportedFormat(const VkPhysicalDevice& physical_device, const UTL::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physical_device, format, &formatProperties);

        if (tiling == VK_IMAGE_TILING_LINEAR && (formatProperties.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (formatProperties.optimalTilingFeatures & features) == features) {
            return format;
        }
	}
    debug_error("::VULKAN:ERROR Failed to find supported depth format\n");
    return VK_FORMAT_D32_SFLOAT; // fallback
}

const char* formatToString(VkFormat format) {
	switch (format) {
    case VK_FORMAT_UNDEFINED: return "UNDEFINED";
    case VK_FORMAT_R8G8B8A8_UNORM: return "R8G8B8A8_UNORM";
    case VK_FORMAT_B8G8R8A8_UNORM: return "B8G8R8A8_UNORM";
    case VK_FORMAT_R8G8B8A8_SRGB: return "R8G8B8A8_SRGB";
    case VK_FORMAT_B8G8R8A8_SRGB: return "B8G8R8A8_SRGB";
    case VK_FORMAT_A2R10G10B10_UNORM_PACK32: return "A2R10G10B10_UNORM_PACK32";
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32: return "A2B10G10R10_UNORM_PACK32";
    case VK_FORMAT_R16G16B16A16_SFLOAT: return "R16G16B16A16_SFLOAT";
    case VK_FORMAT_D32_SFLOAT: return "D32_SFLOAT";
    case VK_FORMAT_D32_SFLOAT_S8_UINT: return "D32_SFLOAT_S8_UINT";
    case VK_FORMAT_D24_UNORM_S8_UINT: return "D24_UNORM_S8_UINT";
    case VK_FORMAT_D16_UNORM: return "D16_UNORM";
    default: return "UNKNOWN_FORMAT";
	}
}
const char* colorSpaceToString(VkColorSpaceKHR colorSpace) {
    switch (colorSpace) {
    case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR: return "SRGB_NONLINEAR";
    case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT: return "DISPLAY_P3_NONLINEAR";
    case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT: return "EXTENDED_SRGB_LINEAR";
    case VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT: return "DISPLAY_P3_LINEAR";
    case VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT: return "DCI_P3_NONLINEAR";
    case VK_COLOR_SPACE_BT709_LINEAR_EXT: return "BT709_LINEAR";
    case VK_COLOR_SPACE_BT709_NONLINEAR_EXT: return "BT709_NONLINEAR";
    case VK_COLOR_SPACE_BT2020_LINEAR_EXT: return "BT2020_LINEAR";
    case VK_COLOR_SPACE_HDR10_ST2084_EXT: return "HDR10_ST2084";
    case VK_COLOR_SPACE_DOLBYVISION_EXT: return "DOLBYVISION";
    case VK_COLOR_SPACE_HDR10_HLG_EXT: return "HDR10_HLG";
    case VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT: return "ADOBERGB_LINEAR";
    case VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT: return "ADOBERGB_NONLINEAR";
    case VK_COLOR_SPACE_PASS_THROUGH_EXT: return "PASS_THROUGH";
    case VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT: return "EXTENDED_SRGB_NONLINEAR";
    default: return "UNKNOWN_COLOR_SPACE";
    }
}
const char* presentModeToString(VkPresentModeKHR presentMode) {
    switch (presentMode) {
    case VK_PRESENT_MODE_IMMEDIATE_KHR: return "IMMEDIATE (No VSync)";
    case VK_PRESENT_MODE_MAILBOX_KHR: return "MAILBOX (Triple Buffer)";
    case VK_PRESENT_MODE_FIFO_KHR: return "FIFO (VSync)";
    case VK_PRESENT_MODE_FIFO_RELAXED_KHR: return "FIFO_RELAXED";
    case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR: return "SHARED_DEMAND_REFRESH";
    case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR: return "SHARED_CONTINUOUS_REFRESH";
    default: return "UNKNOWN_PRESENT_MODE";
    }
}
const char* imageLayoutToString(VkImageLayout layout) {
    switch (layout) {
    case VK_IMAGE_LAYOUT_UNDEFINED: return "UNDEFINED";
    case VK_IMAGE_LAYOUT_GENERAL: return "GENERAL";
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: return "COLOR_ATTACHMENT_OPTIMAL";
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: return "DEPTH_STENCIL_ATTACHMENT_OPTIMAL";
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL: return "DEPTH_STENCIL_READ_ONLY_OPTIMAL";
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: return "SHADER_READ_ONLY_OPTIMAL";
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: return "TRANSFER_SRC_OPTIMAL";
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: return "TRANSFER_DST_OPTIMAL";
    case VK_IMAGE_LAYOUT_PREINITIALIZED: return "PREINITIALIZED";
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: return "PRESENT_SRC_KHR";
    default: return "UNKNOWN_LAYOUT";
    }
}
const char* resultToString(VkResult result) {
    switch (result) {
    case VK_SUCCESS: return "SUCCESS";
    case VK_NOT_READY: return "NOT_READY";
    case VK_TIMEOUT: return "TIMEOUT";
    case VK_EVENT_SET: return "EVENT_SET";
    case VK_EVENT_RESET: return "EVENT_RESET";
    case VK_INCOMPLETE: return "INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY: return "ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED: return "ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST: return "ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED: return "ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT: return "ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT: return "ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT: return "ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER: return "ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS: return "ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED: return "ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL: return "ERROR_FRAGMENTED_POOL";
    case VK_ERROR_SURFACE_LOST_KHR: return "ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case VK_SUBOPTIMAL_KHR: return "SUBOPTIMAL_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR: return "ERROR_OUT_OF_DATE_KHR";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "ERROR_INCOMPATIBLE_DISPLAY_KHR";
    case VK_ERROR_VALIDATION_FAILED_EXT: return "ERROR_VALIDATION_FAILED_EXT";
    case VK_ERROR_INVALID_SHADER_NV: return "ERROR_INVALID_SHADER_NV";
    default: return "UNKNOWN_ERROR";
    }
}

const char* descriptorPoolPolicyToString(descriptorPoolPolicy policy) {
	switch (policy) {
    case descriptorPoolPolicy::NeverFree: return "NeverFree (Immutable)";
    case descriptorPoolPolicy::BulkReset: return "BulkReset (Per Scene)";
    case descriptorPoolPolicy::PerFrameReset: return "PerFrameReset";
    case descriptorPoolPolicy::Linear: return "Linear (Per Draw + Overflow)";
    case descriptorPoolPolicy::DeferredFree: return "DeferredFree (Hot Reload)";
    default: return "UNKNOWN_POLICY";
	}
}

}

