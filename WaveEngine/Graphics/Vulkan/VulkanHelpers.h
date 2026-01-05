#pragma once

#include "VulkanCommonHeaders.h"

namespace WAVEENGINE::GRAPHICS::VULKAN::VKX {
//////////////////////////////Descriptor Pool Sizes///////////////////////////////////

// Usage: global textures, samplers, immutable resources
// Features: allocated when initializing, never released during the execution
inline UTL::vector<VkDescriptorPoolSize> immutablePoolSizes = {					
	{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2048},			// static texture / material
	{VK_DESCRIPTOR_TYPE_SAMPLER, 128},							// static sampler
	{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 512}					// static image resource
};

// Usage: lights in the scene, environment texture maps, global UBO for scene
// Features: update when the scene is changed
inline UTL::vector<VkDescriptorPoolSize> perScenePoolSizes = {
	{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 128},					// global scene UBO
	{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 256},			// environment map
	{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 64}					// scene data buffer
};

// Usage: camera matrix, UBO for each frame, temporary resources
// Features: update for each frame
inline UTL::vector<VkDescriptorPoolSize> perFramePoolSizes = {
	{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 512},					// camera info, time info
	{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 256},			// dynamic UBO
	{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 128},					// temporary data for compute shader 
	{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 256}			// temporary textures
};

// Usage: model matrix, material params and most frequently updated data
// Features: update multiple times in each frame 
inline UTL::vector<VkDescriptorPoolSize> perDrawPoolSizes = {
	{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024},					// object UBO
	{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 512},			// dynamic object data
	{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 512}			// object materials
};

// Usage: temporary resources
// Features:
inline UTL::vector<VkDescriptorPoolSize> deferredPoolSizes = {
	{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024},			// sampled textures waiting to be released
	{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 512},					// UBO waiting to be released
	{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 256},					// storage buffer waiting to be released
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 128}			// dynamic UBO waiting to be released
};

////////////////////////////////Device Properties////////////////////////////////////

struct QueueFamilyIndices {
	int graphicsFamily = -1;
	int presentFamily = -1;
	int computeFamily = -1;
	int transferFamily = -1;
	bool isComplete() const {
		return graphicsFamily >= 0;
	}

	bool hasPresentSupport() const {
		return presentFamily >= 0;
	}

	bool hasComputeSupport() const {
		return computeFamily >= 0;
	}

	bool hasTransferSupport() const {
		return transferFamily >= 0;
	}
};

u32 rateDeviceSuitability(const VkPhysicalDevice& physical_device);

VkSampleCountFlagBits getMaxUsableSampleCount(const VkPhysicalDevice& physical_device);

QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& physical_device);

QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& physical_device, const VkSurfaceKHR& surface);

VkPhysicalDeviceFeatures2 findPhysicalDeviceFeatures(const VkPhysicalDevice& physical_device);

VkPhysicalDeviceProperties2 findPhysicalDeviceProperties(const VkPhysicalDevice& physical_device);

VkPhysicalDeviceMemoryProperties2 findPhysicalDeviceMemoryProperties(const VkPhysicalDevice& physical_device);

}