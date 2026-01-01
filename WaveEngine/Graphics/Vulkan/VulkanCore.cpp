#include "VulkanCore.h"
#include "VulkanSurface.h"
#include "VulkanSwapChain.h"
#include "VulkanShader.h"

namespace WAVEENGINE::GRAPHICS::VULKAN::CORE {

namespace {

vulkanContext						vk_ctx;

using surfaceCollection = UTL::freeList<vulkanSurface>;
using swapchainCollection = UTL::freeList<vulkanSwapChain>;
surfaceCollection					surfaces;
swapchainCollection					swapchains;

descriptorPool						immutable_pool{};								// never update
descriptorPool						per_scene_pool{};								// infrequently update
descriptorPool						per_frame_pool[frame_buffer_count]{};			// frequently update
descriptorPool						per_draw_pool{};								// most frequently update

u32									deferred_releases_flag[frame_buffer_count];
std::mutex							deferred_releases_mutex{};

// TODO maybe we can move this to specific pass file
UTL::vector<SHADERS::vulkanShader>	shaders{ SHADERS::engineShader::count };

bool check_vulkan_runtime() {
	HMODULE vulkan_dll = LoadLibraryA("vulkan-1.dll");
	if (!vulkan_dll) {
		MessageBoxA(nullptr,
			"Vulkan Runtime not found!\n\n"
			"Please update your GPU drivers or install Vulkan Runtime from:\n"
			"https://vulkan.lunarg.com/sdk/home#windows\n\n"
			"The application will now fall back to Direct3D 12.",
			"Vulkan Required",
			MB_OK | MB_ICONWARNING);
		return false;
	}
	FreeLibrary(vulkan_dll);
	return true;
}

}

namespace DETAIL {

void deferred_release() {
	const u32 frame_idx{ current_frame_index() };
}

}

bool initialize() {
	// check Vulkan Runtime
	if (!check_vulkan_runtime()) {
		return false;
	}

	VKbCall(vk_ctx.initialize(), "::VULKAN:ERROR Failed to initialize Vulkan Context\n");
#ifdef _DEBUG
	vk_ctx.setupDebugMessenger();
#endif

	VKbCall(immutable_pool.initialize(2048, VKX::immutablePoolSizes), "::VULKAN:ERROR Failed to initialize immutable descriptor pool\n");
	VKbCall(per_scene_pool.initialize(256, VKX::perScenePoolSizes), "::VULKAN:ERROR Failed to initialize per scene descriptor pool\n");
	for (u32 i{0}; i < frame_buffer_count; ++i) {
		VKbCall(per_frame_pool[i].initialize(512, VKX::perFramePoolSizes), "::VULKAN:ERROR Failed to initialize per frame descriptor pool\n");
	}
	VKbCall(per_draw_pool.initialize(1024, VKX::perDrawPoolSizes), "::VULKAN:ERROR Failed to initialize per draw descriptor pool\n");

	VKbCall(SHADERS::loadEngineShaders(), "::VULKAN:ERROR Failed to load engine built-in shaders");

	return true;

}

void shutdown() {
	
}

VkPhysicalDevice physical_device() {
	return vk_ctx.physical_device();
}

VkDevice device() {
	return vk_ctx.device();
}

VkQueue graphics_queue() {
	return vk_ctx.graphics_queue().queue();
}

VkQueue present_queue() {
	return vk_ctx.present_queue().queue();
}

u32 current_frame_index() {
	// TODO
	return u32_invalid_id;
}

void set_deferred_releases_flag() {
	
}

surface create_surface(PLATFORM::window window) {
	surface_id id{ surfaces.add(window) };
	surfaces[id].create(vk_ctx.instance_context());

	swapchain_id sc_id { swapchains.add(vk_ctx.physical_device(), vk_ctx.device(), surfaces[id].surface(), window)};
	swapchains[sc_id].create();

	// validate Present Support
	VKX::QueueFamilyIndices indices = VKX::findQueueFamilies(vk_ctx.physical_device(), surfaces[id].surface());
#if _DEBUG
	if (!indices.hasPresentSupport()) {
		debug_error("::VULKAN:ERROR Graphics queue doesn't support present on this surface\n");
		assert(indices.hasPresentSupport());
	}
#endif

	// if we need separate present queue, reinitialize.
	if (indices.presentFamily != vk_ctx.graphics_queue().familyIndex()) {
		vk_ctx.present_queue().initialize(vk_ctx.device(), indices.presentFamily);
#if _DEBUG
		debug_output("::VULKAN:WARNING Using separate present queue\n");
#endif
	}
	return surface{id};
}

void remove_surface(surface_id id) {
	
}

void resize_surface(surface_id id, u32 width, u32 height) {
	
}

u32 surface_width(surface_id id) {
	return 0;
}

u32 surface_height(surface_id id) {
	return 0;
}

void render_surface(surface_id id) {
	
}
}
