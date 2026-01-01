#include "VulkanResources.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

bool descriptorPool::initialize(VkDevice device, VkDescriptorPoolCreateInfo& createInfo) {
	_device = device;
	std::lock_guard lock{ _mutex };
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

	VKCall(vkCreateDescriptorPool(_device, &createInfo, nullptr, &_pool), "::VULKAN:ERROR Failed to create descriptor pool");
	return true;
}

bool descriptorPool::initialize(VkDevice device, u32 max_sets, VkDescriptorPoolCreateFlags flags) {
	return initialize(device, max_sets, _pool_sizes, flags);
}

bool descriptorPool::initialize(VkDevice device, u32 max_sets, const UTL::vector<VkDescriptorPoolSize>& pool_Sizes, VkDescriptorPoolCreateFlags flags) {
	VkDescriptorPoolCreateInfo createInfo{};

	createInfo.flags = flags;
	createInfo.maxSets = max_sets;
	createInfo.poolSizeCount = static_cast<u32>(pool_Sizes.size());
	createInfo.pPoolSizes = pool_Sizes.data();

	return initialize(device, createInfo);
}

void descriptorPool::release() {
	assert(!_size);
	// TODO not deferred
	VK_DESTROY_PTR_BY(vkDestroyDescriptorPool, _device, _pool);
}

void descriptorPool::process_deferred_free(u32 frame_idx) {
	std::lock_guard lock{ _mutex };
	// TODO deferred_free
}

descriptorSet descriptorPool::allocate(descriptorSetLayout layout) {
	std::lock_guard lock{ _mutex };

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = _pool;
	allocInfo.descriptorSetCount = 1;
	VkDescriptorSetLayout vkSetLayout = layout.layout();
	allocInfo.pSetLayouts = &vkSetLayout;
	
	VkDescriptorSet vkSet{ VK_NULL_HANDLE };
	VKCall(vkAllocateDescriptorSets(_device, &allocInfo, &vkSet), "::VULKAN:ERROR Failed to allocate descriptor set");
	
	return descriptorSet{ vkSet };
 }

UTL::vector<descriptorSet> descriptorPool::allocate(const UTL::vector<descriptorSetLayout>& layouts) {
	std::lock_guard lock{ _mutex };

	UTL::vector<VkDescriptorSetLayout> vkSetLayouts;
	for (const auto& layout : layouts) {
		vkSetLayouts.emplace_back(layout.layout());
	}
	
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = _pool;
	allocInfo.descriptorSetCount = static_cast<u32>(vkSetLayouts.size());
	allocInfo.pSetLayouts = vkSetLayouts.data();

	UTL::vector<VkDescriptorSet> vkSets(vkSetLayouts.size(), VK_NULL_HANDLE);
	VKCall(vkAllocateDescriptorSets(_device, &allocInfo, vkSets.data()), "::VULKAN:ERROR Failed to allocate descriptor sets");

	UTL::vector<descriptorSet> result;
	result.reserve(vkSets.size());
	for (auto vkSet : vkSets) {
		result.emplace_back(descriptorSet{ vkSet });
	}

	return result;
}

void descriptorPool::free(descriptorSet& descSet, u32 current_frame_index) {
	
}

}
