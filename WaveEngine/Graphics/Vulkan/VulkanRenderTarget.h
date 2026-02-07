#pragma once
#include "VulkanCommonHeaders.h"
#include "VulkanImage.h"
#include "VulkanMemory.h"
#include "VulkanRenderPass.h"
#include "VulkanSwapChain.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

struct renderTargetDesc {
	u32							width;
	u32							height;
	VkSampleCountFlagBits			samples;

	struct colorAttachment {
		VkFormat					format;
		VkImageUsageFlags			usage;
	};

	UTL::vector<colorAttachment>	colors;

	bool							withDepth{ false };
	VkFormat						depthFormat{ VK_FORMAT_D32_SFLOAT };
};

class vulkanRenderTarget {
private:
	u32							_width{};
	u32							_height{};
	VkSampleCountFlagBits			_samples{ VK_SAMPLE_COUNT_1_BIT };

	UTL::vector<vulkanImageMemory>	_color_images{};
	UTL::vector<vulkanImageView>	_color_image_views{};
	UTL::vector<VkFormat>			_color_formats{};

	vulkanImageMemory				_depth_image{};
	vulkanImageView				_depth_image_view{};
	VkFormat						_depth_format{};

public:
	VkResult create(const VkPhysicalDevice& physical_device, const VkDevice& device, const renderTargetDesc& desc);
	void destroy();

	constexpr u32 width() const { return _width; }
	constexpr u32 height() const { return _height; }
	[[nodiscard]] u32 colorCount() const { return static_cast<u32>(_color_image_views.size()); }
	[[nodiscard]] bool hasDepth() const { return _depth_image_view.isValid(); }

	[[nodiscard]] VkImage colorImage(u32 idx) const {
		assert(idx < _color_images.size());
		return _color_images[idx].image();
	}
	[[nodiscard]] const vulkanImageView& imageView(u32 idx) const {
		assert(idx < _color_image_views.size());
		return _color_image_views[idx];
	}
	[[nodiscard]] constexpr VkFormat colorFormat(u32 idx) const {
		assert(idx < _color_formats.size());
		return _color_formats[idx];
	}

	[[nodiscard]] VkImage depthImage() const { return _depth_image.image(); }
	[[nodiscard]] const vulkanImageView& depthView() const { return _depth_image_view; }
	[[nodiscard]] constexpr VkFormat depthFormat() const { return _depth_format; }
};

}