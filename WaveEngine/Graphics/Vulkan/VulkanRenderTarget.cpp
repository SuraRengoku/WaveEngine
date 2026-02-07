#include "VulkanRenderTarget.h"
#include "VulkanContext.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

namespace {

VkImageAspectFlags depth_aspect_flags(VkFormat format) {
	switch (format) {
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
	case VK_FORMAT_D24_UNORM_S8_UINT:
		return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	default:
		return VK_IMAGE_ASPECT_DEPTH_BIT;
	}
}

}

VkResult vulkanRenderTarget::create(const VkPhysicalDevice& physical_device, const VkDevice& device, const renderTargetDesc& desc) {
	destroy();

	assert(desc.width > 0 && desc.height > 0);

	_width = desc.width;
	_height = desc.height;
	_samples = desc.samples;
	
	deviceContext dCtx{ device, {}, {}, nullptr };

	_color_images.resize(desc.colors.size());
	_color_image_views.resize(desc.colors.size());
	_color_formats.reserve(desc.colors.size());

	VkComponentMapping components{};
	VkImageSubresourceRange colorRange{};
	colorRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	colorRange.baseMipLevel = 0;
	colorRange.levelCount = 1;
	colorRange.baseArrayLayer = 0;
	colorRange.layerCount = 1;

	for (u32 i{ 0 }; i < desc.colors.size(); ++i) {
		const auto& color = desc.colors[i];
		_color_formats.push_back(color.format);

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = color.format;
		imageInfo.extent = { _width, _height, 1 };
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = _samples;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = color.usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		if (VkResult result = _color_images[i].create(device, physical_device, imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
			debug_error("::VULKAN:ERROR Failed to create render target color image\n");
			destroy();
			return result;
		}

		_color_image_views[i].create(dCtx, _color_images[i].image(), color.format, VK_IMAGE_VIEW_TYPE_2D, components, colorRange);
	}

	if (desc.withDepth) {
		_depth_format = desc.depthFormat;

		VkImageCreateInfo depthInfo{};
		depthInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		depthInfo.imageType = VK_IMAGE_TYPE_2D;
		depthInfo.format = _depth_format;
		depthInfo.extent = { _width, _height, 1 };
		depthInfo.mipLevels = 1;
		depthInfo.arrayLayers = 1;
		depthInfo.samples = _samples;
		depthInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		depthInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		depthInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		depthInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		if (VkResult result = _depth_image.create(device, physical_device, depthInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
			debug_error("::VULKAN:ERROR Failed to create render target depth image\n");
			destroy();
			return result;
		}

		VkImageSubresourceRange depthRange{};
		depthRange.aspectMask = depth_aspect_flags(_depth_format);
		depthRange.baseMipLevel = 0;
		depthRange.levelCount = 1;
		depthRange.baseArrayLayer = 0;
		depthRange.layerCount = 1;

		_depth_image_view.create(dCtx, _depth_image.image(), _depth_format, VK_IMAGE_VIEW_TYPE_2D, components, depthRange);
	} else {
		_depth_format = VK_FORMAT_UNDEFINED;
		_depth_image_view.destroy();
		_depth_image = vulkanImageMemory();
	}

#ifdef _DEBUG
	debug_output("::VULKAN:INFO Render target created (%ux%u)\n", _width, _height);
#endif
}

void vulkanRenderTarget::destroy() {
	for (auto& view : _color_image_views) {
		view.destroy();
	}
	_color_image_views.clear();
	_color_images.clear();
	_color_formats.clear();
	
	_depth_image_view.destroy();
	_depth_image = vulkanImageMemory();
	_depth_format = VK_FORMAT_UNDEFINED;

	_width = 0;
	_height = 0;
	_samples = VK_SAMPLE_COUNT_1_BIT;

#ifdef _DEBUG
	debug_output("::VULKAN:INFO Render target destroyed\n");
#endif
}

}