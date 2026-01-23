#pragma once
#include "VulkanCommonHeaders.h"
#include "VulkanContext.h"
#if _WIN32
#include <vulkan/vulkan_win32.h>
#elif defined(__APPLE__)
#include <vulkan/vulkan_metal.h>
#elif defined(__linux__)
#include <vulkan/vulkan_xlib.h>
#include <vulkan/vulkan_wayland.h>
#elif defined(__ANDROID__)
#include <vulkan/vulkan_android.h>
#endif

namespace WAVEENGINE::GRAPHICS::VULKAN {
	
class vulkanSurface {
public:
	explicit vulkanSurface(PLATFORM::window window) : _window(window) {
		assert(_window.handle());
	}

#if USE_STL_VECTOR
	DISABLE_COPY(vulkanSurface)
#else
	DISABLE_COPY_AND_MOVE(vulkanSurface)
#endif

	~vulkanSurface() {
		assert(_surface == VK_NULL_HANDLE && "vulkanSurface must be explicitly destroyed before destruction");
	}

	bool create(const instanceContext& ctx);
	void destroy(const instanceContext& ctx);

	[[nodiscard]] constexpr const VkSurfaceKHR& surface() const { return _surface; }
	[[nodiscard]] constexpr const PLATFORM::window& window() const { return _window; }

private:
	VkSurfaceKHR			_surface{ VK_NULL_HANDLE };
	PLATFORM::window		_window{};
};

}