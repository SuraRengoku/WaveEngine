#pragma once
#include "VulkanCommonHeaders.h"
#include "VulkanContext.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

struct descriptorSetLayoutImpl {
	VkDevice                device{ VK_NULL_HANDLE };
	VkDescriptorSetLayout   layout{ VK_NULL_HANDLE };

	~descriptorSetLayoutImpl() {
		if (layout != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(device, layout, nullptr);
		}
	}
};

class vulkanDescriptorSetLayout {
public:
    vulkanDescriptorSetLayout() = default;

    // non-owning view（optional）
    explicit vulkanDescriptorSetLayout(VkDescriptorSetLayout layout) {
        auto impl = std::make_shared<descriptorSetLayoutImpl>();
        impl->layout = layout;
        _impl = impl;
    }

    vulkanDescriptorSetLayout(const deviceContext& dCtx, const VkDescriptorSetLayoutCreateInfo& createInfo) {
        create(dCtx, createInfo);
    }

    vulkanDescriptorSetLayout(const deviceContext& dCtx,
                         u32 bindingCount, const VkDescriptorSetLayoutBinding* pBindings,
                         VkDescriptorSetLayoutCreateFlags flags = 0, const void* next = nullptr) {
        create(dCtx, bindingCount, pBindings, flags, next);
    }

    [[nodiscard]] VkDescriptorSetLayout layout() const {
        return _impl ? _impl->layout : VK_NULL_HANDLE;
    }
    [[nodiscard]] bool is_valid() const {
        return _impl && _impl->layout != VK_NULL_HANDLE;
    }

    VkResult create(const deviceContext& dCtx, const VkDescriptorSetLayoutCreateInfo& createInfo);
    VkResult create(const deviceContext& dCtx,
					u32 bindingCount, const VkDescriptorSetLayoutBinding* pBindings,
                    VkDescriptorSetLayoutCreateFlags flags = 0, const void* next = nullptr);

private:
    std::shared_ptr<descriptorSetLayoutImpl> _impl;
};


// non-owning handle
class vulkanDescriptorSetHandle {
public:
	DISABLE_COPY(vulkanDescriptorSetHandle);

	explicit vulkanDescriptorSetHandle(const VkDescriptorSet& set) : _set(set) {}
	vulkanDescriptorSetHandle(vulkanDescriptorSetHandle&& other) noexcept : _set(other.set()) {
		other._set = VK_NULL_HANDLE;
	}

	vulkanDescriptorSetHandle& operator=(vulkanDescriptorSetHandle&& other) noexcept {
		if (this != &other) {
			assert(_set == VK_NULL_HANDLE); // make sure already released
			_set = other._set;
			other._set = VK_NULL_HANDLE;
		}
		return *this;
	}

	void write() const;

	static void update();

	~vulkanDescriptorSetHandle() {
		// wait for deferred release
		assert(_set == VK_NULL_HANDLE);
	}

	[[nodiscard]] constexpr VkDescriptorSet set() const { return _set; }
	[[nodiscard]] constexpr bool is_valid() const { return _set != VK_NULL_HANDLE; }

	// u32							_index{ u32_invalid_id };

private:
	void invalidate() noexcept {
		_set = VK_NULL_HANDLE;
#if _DEBUG
		_container = nullptr;
#endif
	}

	VkDescriptorSet				_set{ VK_NULL_HANDLE };
	friend class vulkanDescriptorPool;
#ifdef _DEBUG
	vulkanDescriptorPool*		_container{ nullptr };
#endif
};

enum class descriptorPoolPolicy : u8 {
	NeverFree,				// immutable
	BulkReset,				// per_scene
	PerFrameReset,			// per_frame
	Linear,					// per_draw (with overflow pool)
	DeferredFree,			// runtime / hot-reload
};

class vulkanDescriptorPool {
public:
	vulkanDescriptorPool() : _policy(descriptorPoolPolicy::PerFrameReset) {}
	explicit vulkanDescriptorPool(descriptorPoolPolicy policy) : _policy(policy) {}
	explicit vulkanDescriptorPool(descriptorPoolPolicy policy,  const UTL::vector<VkDescriptorPoolSize>& poolSizes)
		: _policy(policy), _pool_sizes(poolSizes) {}

	DISABLE_COPY_AND_MOVE(vulkanDescriptorPool)

	~vulkanDescriptorPool() {
		assert(_pool == VK_NULL_HANDLE);
	}

	bool initialize(VkDevice device, VkDescriptorPoolCreateInfo& createInfo);
	bool initialize(VkDevice device, u32 max_sets, VkDescriptorPoolCreateFlags flags = 0);
	bool initialize(VkDevice device, u32 max_sets, const UTL::vector<VkDescriptorPoolSize>& pool_sizes, VkDescriptorPoolCreateFlags flags = 0);

	[[nodiscard]] vulkanDescriptorSetHandle allocate(const vulkanDescriptorSetLayout& layout);
	[[nodiscard]] UTL::vector<vulkanDescriptorSetHandle> allocate(const UTL::vector<vulkanDescriptorSetLayout>& layouts);

	// ====================================== manual Manager =====================================

	// Calling this will invalidate all sets in the pool immediately
	// before calling reset you have to make sure all sets in the pool will no longer be used by GPU
	void reset();

	// free single descriptor set (only works in DeferredFree policy)
	void free(vulkanDescriptorSetHandle& descSet);

	// totally release the pool
	void release();

	// ==================================== automatic Manager ===================================
	
	// called at the beginning of a frame, policy dependent
	void begin_frame(u32 frame_index);
	// called at the end of a frame, optional
	void end_frame(u32 frame_index);

	// batch reset, policy dependent
	void bulk_reset();

	// ===================================== deferred release ===================================

	// put descriptor set into deferred release list
	void deferred_free(vulkanDescriptorSetHandle& descSet, u32 current_frame_index);
	// after frame fence signaled
	void process_deferred_free();

	[[nodiscard]] constexpr VkDescriptorPool pool() const { return _pool; }
	[[nodiscard]] constexpr u32 capacity() const { return _capacity; }
	[[nodiscard]] constexpr u32 size() const { return _size; }
	[[nodiscard]] constexpr descriptorPoolPolicy policy() const { return _policy; }
	[[nodiscard]] constexpr bool is_valid() const { return _pool != VK_NULL_HANDLE; }

private:
	// ========================================== Utils =========================================

	// adjust create flags based on policy
	VkDescriptorPoolCreateFlags get_policy_flags() const;
	
	vulkanDescriptorSetHandle allocate_from_overflow(const vulkanDescriptorSetLayout& layout);
	UTL::vector<vulkanDescriptorSetHandle> allocate_batch_from_overflow(const UTL::vector<vulkanDescriptorSetLayout>& layouts);

	bool create_overflow_pool();

	struct DeferredFreeItem {
		VkDescriptorSet set;
		u32 frame_index;
	};

	descriptorPoolPolicy							_policy;

	VkDescriptorPool								_pool{ VK_NULL_HANDLE };
	VkDevice										_device{ VK_NULL_HANDLE };

	u32												_capacity{ 0 };
	u32												_size{ 0 };
	u32												_current_frame{ 0 };

	const UTL::vector<VkDescriptorPoolSize>			_pool_sizes{};

	// only used when pool is freed deferred
	UTL::vector<DeferredFreeItem>					_deferred_free_queue{};

	UTL::vector<VkDescriptorPool>					_overflow_pools{};
	// number of allocation in the overflow pool
	UTL::vector<u32>								_overflow_sizes{}; 

	std::mutex										_mutex{};


};

}