#include "VulkanResources.h"
#include "VulkanCommonHeaders.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

VkResult vulkanDescriptorSetLayout::create(const deviceContext& dCtx, const VkDescriptorSetLayoutCreateInfo& createInfo) {
	assert(!_impl && "::VULKAN:ERROR DescriptorSetLayout already created\n");

	auto impl = std::make_shared<descriptorSetLayoutImpl>();
	impl->device = dCtx._device;

	if (VkResult result = vkCreateDescriptorSetLayout(
		dCtx._device,
		&createInfo,
		dCtx._allocator,
		&impl->layout
	)) {
		debug_error("::VULKAN:ERROR Failed to create a descriptor set layout\n");
		return result;
	}

	_impl = std::move(impl);
	return VK_SUCCESS;
}

VkResult vulkanDescriptorSetLayout::create(const deviceContext& dCtx, u32 bindingCount,
	const VkDescriptorSetLayoutBinding* pBindings, VkDescriptorSetLayoutCreateFlags flags, const void* next) {
	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.flags = flags;
	createInfo.pNext = next;
	createInfo.bindingCount = bindingCount;
	createInfo.pBindings = pBindings;

	return create(dCtx, createInfo);
}


bool vulkanDescriptorPool::initialize(VkDevice device, VkDescriptorPoolCreateInfo& createInfo) {
	_device = device;
	std::lock_guard lock{ _mutex };
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

	VKCall(vkCreateDescriptorPool(_device, &createInfo, nullptr, &_pool), "::VULKAN:ERROR Failed to create descriptor pool\n");

	_capacity = createInfo.maxSets;
	// _free_handles = std::move(std::make_unique<u32[]>(_capacity));
	_size = 0;

	// for (u32 i{ 0 }; i < _capacity; ++i) {
		// _free_handles[i] = i;
	// } // initially, every slot is free

	return true;
}

bool vulkanDescriptorPool::initialize(VkDevice device, u32 max_sets, VkDescriptorPoolCreateFlags flags) {
	return initialize(device, max_sets, _pool_sizes, flags);
}

bool vulkanDescriptorPool::initialize(VkDevice device, u32 max_sets, const UTL::vector<VkDescriptorPoolSize>& pool_Sizes, VkDescriptorPoolCreateFlags flags) {
	VkDescriptorPoolCreateInfo createInfo{};

	createInfo.flags = flags;
	createInfo.maxSets = max_sets;
	createInfo.poolSizeCount = static_cast<u32>(pool_Sizes.size());
	createInfo.pPoolSizes = pool_Sizes.data();

	return initialize(device, createInfo);
}

void vulkanDescriptorPool::release() {
	assert(!_size);

#if _DEBUG
	assert(_deferred_free_sets.empty());
#endif

	VK_DESTROY_PTR_BY(vkDestroyDescriptorPool, _device, _pool);
	_pool = VK_NULL_HANDLE;
}

vulkanDescriptorSetHandle vulkanDescriptorPool::allocate(vulkanDescriptorSetLayout layout) {
	std::lock_guard lock{ _mutex };
	assert(_pool);

	VKbCall(_size + 1 <= _capacity, "::VULKAN:ERROR Descriptor pool capacity exceeded\n");

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = _pool;
	allocInfo.descriptorSetCount = 1;
	VkDescriptorSetLayout vkSetLayout = layout.layout();
	allocInfo.pSetLayouts = &vkSetLayout;
	
	VkDescriptorSet vkSet{ VK_NULL_HANDLE };
	VKCall(vkAllocateDescriptorSets(_device, &allocInfo, &vkSet), "::VULKAN:ERROR Failed to allocate descriptor set\n");
	
	++_size;
#if _DEBUG
	auto handle = vulkanDescriptorSetHandle{ vkSet };
	handle.container = this;
	return handle;
#else
	return vulkanDescriptorSetHandle{ vkSet };
#endif
}

UTL::vector<vulkanDescriptorSetHandle> vulkanDescriptorPool::allocate(const UTL::vector<vulkanDescriptorSetLayout>& layouts) {
	std::lock_guard lock{ _mutex };
	assert(_pool);

	VKbCall(_size + layouts.size() <= _capacity, "::VULKAN:ERROR Descriptor pool capacity exceeded\n");

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
	VKCall(vkAllocateDescriptorSets(_device, &allocInfo, vkSets.data()), "::VULKAN:ERROR Failed to allocate descriptor sets\n");

	UTL::vector<vulkanDescriptorSetHandle> result;
	result.reserve(vkSets.size());
	for (auto vkSet : vkSets) {
#if _DEBUG
		auto handle = vulkanDescriptorSetHandle{ vkSet };
		handle.container = this;
		result.emplace_back(std::move(handle));
#else
		result.emplace_back(vulkanDescriptorSetHandle{ vkSet });
#endif
	}

	_size += static_cast<u32>(result.size());

	return result;
}

void vulkanDescriptorPool::reset() {
	std::lock_guard lock{ _mutex };

	switch (_policy) {
	case descriptorPoolPolicy::PerFrameReset:
	case descriptorPoolPolicy::BulkReset:
	case descriptorPoolPolicy::Linear:
#if _DEBUG
		assert(_deferred_free_sets.empty());
#endif
		vkResetDescriptorPool(_device, _pool, 0);
		_size = 0;
		break;
	case descriptorPoolPolicy::DeferredFree:
		assert(false && "::VULKAN:ERROR DeferredFree pool must not be reset, it can only be deferred freed\n");
		break;
	case descriptorPoolPolicy::NeverFree:
		assert(false && "::VULKAN:ERROR NevenFree pool must not be reset\n");
		break;
	};
}

void vulkanDescriptorPool::free(vulkanDescriptorSetHandle& descSet) {
	assert(descSet.is_valid());
#if _DEBUG
	assert(descSet.container == this);
#endif

	std::lock_guard lock{ _mutex };

	switch (_policy) {
	case descriptorPoolPolicy::DeferredFree:
		// only after we call process_deferred_free(), the sets are freed.
		_deferred_free_sets.push_back(descSet.set());
		break;
	case descriptorPoolPolicy::BulkReset:
	case descriptorPoolPolicy::Linear:
	case descriptorPoolPolicy::PerFrameReset:
		// use reset() to release all resources
		break;
	case descriptorPoolPolicy::NeverFree:
		assert(false && "::VULKAN:ERROR NeverFree pool must not free individual sets\n");
		break;
	}

	descSet.invalidate();
	if (_policy != descriptorPoolPolicy::DeferredFree) {
		--_size;
	}
}

void vulkanDescriptorPool::process_deferred_free() {
	if (_policy != descriptorPoolPolicy::DeferredFree)	return;

	std::lock_guard lock{ _mutex };

	if(_deferred_free_sets.empty()) return;

	vkFreeDescriptorSets(_device, _pool, static_cast<u32>(_deferred_free_sets.size()), _deferred_free_sets.data());
	// only if we call process_deferred_free can we really release sets when the policy is DeferredFree
	_deferred_free_sets.clear();
	_size -= static_cast<u32>(_deferred_free_sets.size()); 
}

}
