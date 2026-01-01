#pragma once
#include "VulkanCommonHeaders.h"
#include "VulkanCore.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

class descriptorSetLayout {
public:
	explicit descriptorSetLayout(const VkDescriptorSetLayout& layout) : _layout(layout) {}

	[[nodiscard]] constexpr VkDescriptorSetLayout layout() const { return _layout; }
	[[nodiscard]] constexpr bool is_valid() const { return _layout != VK_NULL_HANDLE; }

private:
	VkDescriptorSetLayout		_layout{ VK_NULL_HANDLE };
};


class descriptorSet {
public:
	DISABLE_COPY(descriptorSet);

	explicit descriptorSet(const VkDescriptorSet& set) : _set(set) {}
	descriptorSet(descriptorSet&& other) noexcept : _set(other.set()) {
		other._set = VK_NULL_HANDLE;
	}

	descriptorSet& operator=(descriptorSet&& other) noexcept {
		if (this != &other) {
			assert(_set == VK_NULL_HANDLE); // make sure already released
			_set = other._set;
			other._set = VK_NULL_HANDLE;
		}
		return *this;
	}

	void write() const;

	static void update();

	~descriptorSet() {
		assert(_set == VK_NULL_HANDLE);
	}

	[[nodiscard]] constexpr VkDescriptorSet set() const { return _set; }
	[[nodiscard]] constexpr bool is_valid() const { return _set != VK_NULL_HANDLE; }

	u32							_index{ u32_invalid_id };
private:
	VkDescriptorSet				_set = VK_NULL_HANDLE;
#ifdef _DEBUG
	friend class descriptorPool;
	descriptorPool*				container{ nullptr };
#endif
};


class descriptorPool {
public:
	descriptorPool() = default;
	explicit descriptorPool(const UTL::vector<VkDescriptorPoolSize>& poolSizes) : _pool_sizes(poolSizes) {};
	DISABLE_COPY_AND_MOVE(descriptorPool);

	~descriptorPool() {
		assert(_pool == VK_NULL_HANDLE);
	}

	bool initialize(VkDevice device, VkDescriptorPoolCreateInfo& createInfo);
	bool initialize(VkDevice device, u32 max_sets, VkDescriptorPoolCreateFlags flags = 0);
	bool initialize(VkDevice device, u32 max_sets, const UTL::vector<VkDescriptorPoolSize>& pool_Sizes, VkDescriptorPoolCreateFlags flags = 0);

	void release();

	void process_deferred_free(u32 frame_idx);

	[[nodiscard]] descriptorSet allocate(descriptorSetLayout layout);
	[[nodiscard]] UTL::vector<descriptorSet> allocate(const UTL::vector<descriptorSetLayout>& layouts);
	void free(descriptorSet& descSet, u32 current_frame_index);

	[[nodiscard]] constexpr VkDescriptorPool pool() const { return _pool; }
	[[nodiscard]] constexpr u32 capacity() const { return _capacity; }
	[[nodiscard]] constexpr u32 size() const { return _size; }

private:
	VkDescriptorPool								_pool{ VK_NULL_HANDLE };
	VkDevice										_device{ VK_NULL_HANDLE };
	std::mutex										_mutex{};
	UTL::vector<descriptorSet>						_deferred_free_sets[frame_buffer_count]{};
	u32												_capacity{ 0 };
	u32												_size{ 0 };
	const UTL::vector<VkDescriptorPoolSize>			_pool_sizes{};
};

}