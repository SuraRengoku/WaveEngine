#include "VulkanHelpers.h"

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

}

