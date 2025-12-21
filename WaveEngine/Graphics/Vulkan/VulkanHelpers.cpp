#include "VulkanHelpers.h"

namespace  WAVEENGINE::GRAPHICS::VULKAN::VKX {

u32 rateDeviceSuitability(const VkPhysicalDevice& device) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    // for texture compressing, multi viewport rendering
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

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

VkSampleCountFlagBits getMaxUsableSampleCount(const VkPhysicalDevice& device) {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(device, &physicalDeviceProperties);

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

QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& device) {
    QueueFamilyIndices indices;
    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    UTL::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

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

QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& device, const VkSurfaceKHR& surface) {
    QueueFamilyIndices indices;
    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    UTL::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (u32 i{0}; i < queueFamilyCount; ++i) {
        const auto& queueFamily = queueFamilies[i];

        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = static_cast<int>(i);
        }
        VkBool32 presentSupport = false;
        // check if the i-queue in device support the surface
        // which is important to make sure before setting swap chain
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
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

}

