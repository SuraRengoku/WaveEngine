#pragma once
#include "VulkanCommonHeaders.h"
#include "VulkanResources.h"
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
	constexpr static u32 buffer_count{ 3 };

	explicit vulkanSurface(PLATFORM::window window) : _window(window) {
		assert(_window.handle());
	}

#if USE_STL_VECTOR
	DISABLE_COPY(vulkanSurface);
#else

#endif

	DISABLE_COPY_AND_MOVE(vulkanSurface);

	~vulkanSurface() { release(); }

	bool create_surface(VkInstance instance);

	void present() const;

	void resize();

	[[nodiscard]] constexpr VkSurfaceKHR surface() const { return _surface; }

private:
	void finalize();
	void release();

	struct render_target_data {

	};

	VkSurfaceKHR			_surface{ VK_NULL_HANDLE };
	render_target_data		_render_target_data[buffer_count];
	PLATFORM::window		_window{};
	mutable u32				_current_bb_index{ 0 };
};

}