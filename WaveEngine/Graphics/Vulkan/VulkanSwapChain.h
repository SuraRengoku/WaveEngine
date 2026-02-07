#pragma once
#include "VulkanCommonHeaders.h"
#include "VulkanImage.h"
#include "VulkanFramebuffer.h"
#include "VulkanMemory.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

DEFINE_TYPED_ID(swapchain_id);

namespace DETAIL {

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	UTL::vector<VkSurfaceFormatKHR> formats;
	UTL::vector<VkPresentModeKHR> presentModes;
};

}

class vulkanSwapChain {
public:
	constexpr static u32 buffer_count{ frame_buffer_count };
	constexpr static VkFormat default_back_buffer_format{ VK_FORMAT_UNDEFINED };

	vulkanSwapChain() = default;

	explicit vulkanSwapChain(const VkPhysicalDevice& physical_device, const VkDevice& device, const VkSurfaceKHR& surface, const PLATFORM::window& window)
		: _physical_device(physical_device), _device(device), _surface(surface), _window(window) {
		assert(_window.handle());
	}

	DISABLE_COPY_AND_MOVE(vulkanSwapChain)

	~vulkanSwapChain() { release(); }

	void create();	// query + choose + create
	void recreate();	// resizing ...
	void release();

	[[nodiscard]] const VkSurfaceFormatKHR& surfaceFormat() const { return _surface_format; }
	[[nodiscard]] const VkPresentModeKHR& presentMode() const { return _present_mode; }
	[[nodiscard]] const VkExtent2D& extent() const { return _extent; }
	[[nodiscard]] const VkSwapchainKHR& swapchain() const { return _swap_chain; }

	[[nodiscard]] VkFormat format() const { return _surface_format.format; }
	[[nodiscard]] VkColorSpaceKHR colorSpace() const { return _surface_format.colorSpace; }
	[[nodiscard]] u32 width() const { return _extent.width; }
	[[nodiscard]] u32 height() const { return _extent.height; }
	[[nodiscard]] u32 imageCount() const { return static_cast<u32>(_color_images.size()); }

	const UTL::vector<VkImage>& images() const { return _color_images; }
	const UTL::vector<vulkanImageView>& imageViews() const { return _color_image_views; }

	[[nodiscard]] VkImage image(u32 idx) const {
		assert(idx < _color_images.size());
		return _color_images[idx];
	}
	[[nodiscard]] const vulkanImageView& imageView(u32 idx) const {
		assert(idx < _color_image_views.size());
		return _color_image_views[idx];
	}

	[[nodiscard]] bool isValid() const noexcept { return _swap_chain != VK_NULL_HANDLE; }
private:
	void createImageViews();
	void destroyImageViews();

	DETAIL::SwapChainSupportDetails querySwapChainSupport() const;
	void chooseSwapSurfaceFormat(const UTL::vector<VkSurfaceFormatKHR>&);
	void chooseSwapPresentMode(const UTL::vector<VkPresentModeKHR>&);
	void chooseSwapExtent(const VkSurfaceCapabilitiesKHR&);

	VkSwapchainKHR						_swap_chain{ VK_NULL_HANDLE };

	VkPhysicalDevice						_physical_device{ VK_NULL_HANDLE };
	VkDevice								_device{ VK_NULL_HANDLE };
	VkSurfaceKHR							_surface{ VK_NULL_HANDLE };
	PLATFORM::window						_window{};

	VkSurfaceFormatKHR					_surface_format{};
	VkPresentModeKHR						_present_mode{};
	VkExtent2D							_extent{};

	// Colors are default
	// VkImages of swap chain will be managed by GPU driver,
	UTL::vector<VkImage>					_color_images{};
	// However, we have to manually create and destroy the image views
	UTL::vector<vulkanImageView>			_color_image_views{};
};

}
