#include "VulkanResources.h"
#include "VulkanCommonHeaders.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

////////////////////////////////// vulkanDescriptorSetLayout //////////////////////////////

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

/////////////////////////////////// vulkanDescriptorPool //////////////////////////////////

VkDescriptorPoolCreateFlags vulkanDescriptorPool::get_policy_flags() const {
	VkDescriptorPoolCreateFlags flags = 0;
	
	switch (_policy) {
	case descriptorPoolPolicy::PerFrameReset:
	case descriptorPoolPolicy::BulkReset:
		// fully reset allowed
		// NOTE: VK_DESCRIPTOR_POOL_CREATE_RESET_COMMAND_BUFFER_BIT does not exist
		// reset pool does not need extra flags
		break;
	case descriptorPoolPolicy::DeferredFree:
		// single descriptor set release allowed
		flags |= VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		break;
	case descriptorPoolPolicy::NeverFree:
	case descriptorPoolPolicy::Linear:
		// no special flags needed
		break;
	}
	return flags;
}

bool vulkanDescriptorPool::initialize(VkDevice device, VkDescriptorPoolCreateInfo& createInfo) {
	assert(device != VK_NULL_HANDLE);
	
	{
		_device = device;
		_capacity = createInfo.maxSets;
		createInfo.flags |= get_policy_flags();
	}

	std::lock_guard lock{ _mutex };

	if (VkResult result = vkCreateDescriptorPool(_device, &createInfo, nullptr, &_pool)) {
		debug_error("::VULKAN:ERROR Failed to create descriptor pool\n");
		return false;
	}

#ifdef _DEBUG
	debug_output("::VULKAN:INFO Descriptor pool created (policy: %u, maxSets: %u)\n",
		static_cast<u32>(_policy), _capacity);
#endif

	return true;
}

bool vulkanDescriptorPool::initialize(VkDevice device, u32 max_sets, VkDescriptorPoolCreateFlags flags) {
	assert(!_pool_sizes.empty() && "Pool sizes must be set in a constructor or passed to initialize");
	return initialize(device, max_sets, _pool_sizes, flags);
}

bool vulkanDescriptorPool::initialize(VkDevice device, u32 max_sets, const UTL::vector<VkDescriptorPoolSize>& pool_sizes, VkDescriptorPoolCreateFlags flags) {
	assert(device != VK_NULL_HANDLE);
	assert(!pool_sizes.empty());

	_device = device;
	_capacity = max_sets;
	
	flags |= get_policy_flags();

	VkDescriptorPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.flags = flags;
	createInfo.maxSets = max_sets;
	createInfo.poolSizeCount = static_cast<u32>(pool_sizes.size());
	createInfo.pPoolSizes = pool_sizes.data();

	return initialize(device, createInfo);
}

vulkanDescriptorSetHandle vulkanDescriptorPool::allocate(const vulkanDescriptorSetLayout& layout) {
	std::lock_guard lock{ _mutex };
	assert(_pool != VK_NULL_HANDLE);
	assert(layout.is_valid());

	// currently we use overflow pool, so there is theoretically no capacity limits
	//VKbCall(_size + 1 <= _capacity, "::VULKAN:ERROR Descriptor pool capacity exceeded\n");

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = _pool;
	allocInfo.descriptorSetCount = 1;
	VkDescriptorSetLayout vkSetLayout = layout.layout();
	allocInfo.pSetLayouts = &vkSetLayout;
	
	VkDescriptorSet vkSet{ VK_NULL_HANDLE };
	VkResult result = vkAllocateDescriptorSets(_device, &allocInfo, &vkSet);
	
	if (result == VK_SUCCESS) {
		++_size;
		vulkanDescriptorSetHandle handle(vkSet);
#if _DEBUG
		handle._container = this;
#endif
		return handle;
	}

	// Linear policy: if the main is full, try to allocate from overflow pool
	if (_policy == descriptorPoolPolicy::Linear && result == VK_ERROR_OUT_OF_POOL_MEMORY) {
#ifdef _DEBUG
		debug_output("::VULKAN:WARNING Main descriptor pool full, trying overflow pool\n");
#endif
		return allocate_from_overflow(layout);
	}

	debug_error("::VULKAN:ERROR Failed to allocate descriptor set (result: %d)\n", static_cast<s32>(result));
	return vulkanDescriptorSetHandle(VK_NULL_HANDLE);
}

UTL::vector<vulkanDescriptorSetHandle> vulkanDescriptorPool::allocate(const UTL::vector<vulkanDescriptorSetLayout>& layouts) {
	std::lock_guard lock{ _mutex };
	assert(_pool != VK_NULL_HANDLE);

	// currently we use overflow pool, so there is theoretically no capacity limits
	//VKbCall(_size + layouts.size() <= _capacity, "::VULKAN:ERROR Descriptor pool capacity exceeded\n");

	UTL::vector<VkDescriptorSetLayout> vkSetLayouts;
	vkSetLayouts.reserve(layouts.size());
	for (const auto& layout : layouts) {
		assert(layout.is_valid());
		vkSetLayouts.emplace_back(layout.layout());
	}
	
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = _pool;
	allocInfo.descriptorSetCount = static_cast<u32>(vkSetLayouts.size());
	allocInfo.pSetLayouts = vkSetLayouts.data();

	UTL::vector<VkDescriptorSet> vkSets(vkSetLayouts.size(), VK_NULL_HANDLE);
	VkResult result = vkAllocateDescriptorSets(_device, &allocInfo, vkSets.data());

	UTL::vector<vulkanDescriptorSetHandle> handles;
	if (result == VK_SUCCESS) {
		_size += static_cast<u32>(vkSets.size());
		handles.reserve(vkSets.size());
		for (auto vkSet : vkSets) {
			vulkanDescriptorSetHandle handle(vkSet);
#if _DEBUG
			handle._container = this;
#endif
			handles.emplace_back(std::move(handle));
		}
		return handles;
	}

	if (_policy == descriptorPoolPolicy::Linear && result == VK_ERROR_OUT_OF_POOL_MEMORY) {
#ifdef _DEBUG
		debug_output("::VULKAN:WARNING Main descriptor pool full during batch allocation, using overflow pool\n");
#endif
		return allocate_batch_from_overflow(layouts);
	}

	debug_error("::VULKAN:ERROR Failed to allocate descriptor sets (result: %d)\n", static_cast<s32>(result));
	return handles;
}

void vulkanDescriptorPool::reset() {
	assert(_pool != VK_NULL_HANDLE);

	if (_policy == descriptorPoolPolicy::NeverFree) {
#ifdef _DEBUG
		debug_output("::VULKAN:WARNING Attempting to reset NeverFree descriptor pool\n");
#endif
		return;
	}

	std::lock_guard lock{ _mutex };

	VkResult result = vkResetDescriptorPool(_device, _pool, 0);
	if (result == VK_SUCCESS) {
		_size = 0;

		// clear deferred queue because all sets have been reset
		_deferred_free_queue.clear();

#ifdef _DEBUG
		debug_output("::VULKAN:INFO Descriptor pool reset\n");
#endif
	} else {
		debug_error("::VULKAN:ERROR Failed to reset descriptor pool\n");
	}
}

void vulkanDescriptorPool::free(vulkanDescriptorSetHandle& descSet) {
	if (!descSet.is_valid()) return;

#if _DEBUG
	assert(descSet._container == this);
#endif

	if (_policy != descriptorPoolPolicy::DeferredFree) {
#if _DEBUG
		debug_output("::VULKAN:WARNING Attempting to free descriptor set from non-DeferredFree pool\n");
#endif
		return;
	}

	std::lock_guard lock{ _mutex };

	VkDescriptorSet set = descSet.set();
	VkResult result = vkFreeDescriptorSets(_device, _pool, 1, &set);

	if (result == VK_SUCCESS) {
		--_size;
		descSet.invalidate();
#ifdef _DEBUG
		debug_output("::VULKAN:INFO Descriptor set freed\n");
#endif
	} else {
		debug_error("::VULKAN:ERROR Failed to free descriptor set\n");
	}
}

void vulkanDescriptorPool::deferred_free(vulkanDescriptorSetHandle& descSet, u32 current_frame_index) {
	assert(_policy == descriptorPoolPolicy::DeferredFree);
	assert(descSet.is_valid());

	std::lock_guard lock{ _mutex };
	_deferred_free_queue.push_back({ descSet.set(), current_frame_index });
	descSet.invalidate();

#ifdef _DEBUG
	debug_output("::VULKAN:INFO Descriptor set queued for deferred free{frame: %u)\n", current_frame_index);
#endif
}

void vulkanDescriptorPool::process_deferred_free() {
	if (_policy != descriptorPoolPolicy::DeferredFree)	return;

	std::lock_guard lock{ _mutex };

	// release old descriptor sets (exceed frame_buffer_count)
	constexpr u32 safe_frame_lag = frame_buffer_count;

	UTL::vector<VkDescriptorSet> sets_to_free;
	UTL::vector<DeferredFreeItem> remaining_items;

	for (const auto& item : _deferred_free_queue) {
		u32 age = _current_frame >= item.frame_index ?
			(_current_frame - item.frame_index) :
			(UINT32_MAX - item.frame_index + _current_frame);

		if (age >= safe_frame_lag) {
			sets_to_free.push_back(item.set);
		} else {
			remaining_items.push_back(item);
		}
	}

	if (!sets_to_free.empty()) {
		VkResult result = vkFreeDescriptorSets(_device, _pool,
			static_cast<u32>(sets_to_free.size()), sets_to_free.data());
		if (result == VK_SUCCESS) {
			_size -= static_cast<u32>(sets_to_free.size());
#ifdef _DEBUG
			debug_output("::VULKAN:INFO Processed deferred free: %zu descriptor sets\n", sets_to_free.size());
#endif
			debug_error("::VULKAN::ERROR Failed to free deferred descriptor sets\n");
		}
	}
	_deferred_free_queue = std::move(remaining_items);
}

void vulkanDescriptorPool::begin_frame(u32 frame_index) {
	_current_frame = frame_index;

	switch (_policy) {
	case descriptorPoolPolicy::PerFrameReset:
		// reset at the beginning of each frame
		reset();
		break;
	case descriptorPoolPolicy::DeferredFree:
		process_deferred_free();
		break;
	case descriptorPoolPolicy::NeverFree:
	case descriptorPoolPolicy::BulkReset:
	case descriptorPoolPolicy::Linear:
		// skip
		break;
	}
}

void vulkanDescriptorPool::end_frame(u32 frame_index) {
	// most policy do nothing when a frame is finished
	// TODO: add extension
	return;
}

void vulkanDescriptorPool::bulk_reset() {
	if (_policy != descriptorPoolPolicy::BulkReset) {
#ifdef _DEBUG
		debug_output("::VULKAN:WARNING Calling bulk_reset on non-BulkReset pool\n");
#endif
	}
	reset();
}

void vulkanDescriptorPool::release() {
	if (_pool != VK_NULL_HANDLE) {
		// process all deferred release
		if (_policy == descriptorPoolPolicy::DeferredFree) {
			process_deferred_free();
		}

		vkDestroyDescriptorPool(_device, _pool, nullptr);
		_pool = VK_NULL_HANDLE;

#ifdef _DEBUG
		debug_output("::VULKAN:INFO Main descriptor pool released\n");
#endif
	}

	// release all overflow pools
	for (auto overflow_pool : _overflow_pools) {
		if (overflow_pool != VK_NULL_HANDLE) {
			vkDestroyDescriptorPool(_device, overflow_pool, nullptr);
		}
	}
	_overflow_pools.clear();
	_overflow_sizes.clear();

	_size = 0;
	_capacity = 0;
	_device = VK_NULL_HANDLE;
}

// =================================== OVERFLOW POOL SUPPORT ===================================

bool vulkanDescriptorPool::create_overflow_pool() {
	assert(_policy == descriptorPoolPolicy::Linear);
	assert(!_pool_sizes.empty());

	VkDescriptorPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.maxSets = _capacity;
	createInfo.poolSizeCount = static_cast<u32>(_pool_sizes.size());
	createInfo.pPoolSizes = _pool_sizes.data();
	createInfo.flags = get_policy_flags();

	VkDescriptorPool overflow_pool = VK_NULL_HANDLE;
	VkResult result = vkCreateDescriptorPool(_device, &createInfo, nullptr, &overflow_pool);

	if (result == VK_SUCCESS) {
		_overflow_pools.push_back(overflow_pool);
		_overflow_sizes.push_back(0);

#ifdef _DEBUG
		debug_output("::VULKAN:INFO Created overflow descriptor pool #%zu\n", _overflow_pools.size());
#endif
		return true;
	}

	debug_error("::VULKAN:ERROR Failed to create overflow descriptor pool\n");
	return false;
}

vulkanDescriptorSetHandle vulkanDescriptorPool::allocate_from_overflow(const vulkanDescriptorSetLayout& layout) {
	assert(_policy == descriptorPoolPolicy::Linear);

	// try to allocate from overflow pool 
	for (size_t i = 0; i < _overflow_pools.size(); ++i) {
		if (_overflow_sizes[i] < _capacity) {
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = _overflow_pools[i];
			allocInfo.descriptorSetCount = 1;
			VkDescriptorSetLayout vkLayout = layout.layout();
			allocInfo.pSetLayouts = &vkLayout;

			VkDescriptorSet set = VK_NULL_HANDLE;
			VkResult result = vkAllocateDescriptorSets(_device, &allocInfo, &set);

			if (result == VK_SUCCESS) {
				++_overflow_sizes[i];
				vulkanDescriptorSetHandle handle(set);
#ifdef _DEBUG
				handle._container = this;
#endif
				return handle;
			}
		}
	}

	// all overflow pools are full, create new
	if (create_overflow_pool()) {
		return allocate_from_overflow(layout); // recursive
	}

	return vulkanDescriptorSetHandle(VK_NULL_HANDLE);
}

UTL::vector<vulkanDescriptorSetHandle> vulkanDescriptorPool::allocate_batch_from_overflow(
	const UTL::vector<vulkanDescriptorSetLayout>& layouts) {
	assert(_policy == descriptorPoolPolicy::Linear);

	UTL::vector<VkDescriptorSetLayout> vkSetLayouts;
	vkSetLayouts.reserve(layouts.size());
	for (const auto& layout : layouts) {
		vkSetLayouts.emplace_back(layout.layout());
	}

	for (size_t i = 0; i < _overflow_pools.size(); ++i) {
		u32 available_space = _capacity - _overflow_sizes[i];
		
		// if this overflow pool has enough space
		if (available_space >= layouts.size()) {
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = _overflow_pools[i];
			allocInfo.descriptorSetCount = static_cast<u32>(vkSetLayouts.size());
			allocInfo.pSetLayouts = vkSetLayouts.data();

			UTL::vector<VkDescriptorSet> vkSets(vkSetLayouts.size(), VK_NULL_HANDLE);
			VkResult result = vkAllocateDescriptorSets(_device, &allocInfo, vkSets.data());

			if (result == VK_SUCCESS) {
				_overflow_sizes[i] += static_cast<u32>(vkSets.size());

				UTL::vector<vulkanDescriptorSetHandle> handles;
				handles.reserve(vkSets.size());
				for (auto vkSet : vkSets) {
					vulkanDescriptorSetHandle handle(vkSet);
#ifdef _DEBUG
					handle._container = this;
#endif
					handles.emplace_back(std::move(handle));
				}
#ifdef _DEBUG
				debug_output("::VULKAN:INFO Batch allocated {} descriptor sets from overflow pool #%zu\n",
					vkSets.size(), i);
#endif
			}
		}
	}

	if (create_overflow_pool()) {
#ifdef _DEBUG
		debug_output("::VULKAN:INFO Created new overflow pool for batch allocation\n");
#endif
		return allocate_batch_from_overflow(layouts);
	}

	debug_error("::VULKAN:ERROR Failed to allocate batch from overflow pools\n");
	return UTL::vector<vulkanDescriptorSetHandle>();
}
}
