#pragma once

#include "VulkanResources.h"
#include <map>
#include "VulkanQueue.h"
#include "VulkanContext.h"
#include "VulkanRenderPass.h"


namespace WAVEENGINE::GRAPHICS::VULKAN {

struct vulkanFrameInfo {
	u32 surface_width{};
	u32 surface_height{};
};

}

namespace WAVEENGINE::GRAPHICS::VULKAN::CORE {

bool initialize();
void shutdown();

template<typename T>
constexpr void release(T*& resource) {
	if (resource) {
		
	}
}

namespace DETAIL {

void deferred_release();

}

template<typename T>
constexpr void deferred_release(T*& resource) {
	if (resource) {
		DETAIL::deferred_release(resource);
		resource = nullptr;
	}
}

VkPhysicalDevice physical_device();
VkDevice device();

VkQueue graphics_queue();
VkQueue present_queue();

u32 current_frame_index();

void set_deferred_releases_flag();

surface create_surface(PLATFORM::window window);
void remove_surface(surface_id id);
void resize_surface(surface_id id, u32 width, u32 height);
u32 surface_width(surface_id id);
u32 surface_height(surface_id id);
void render_surface(surface_id id);

}