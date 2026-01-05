#include "VulkanCore.h"
#include "VulkanSurface.h"
#include "VulkanSwapChain.h"
#include "VulkanShader.h"
#include "VulkanCommand.h"

namespace WAVEENGINE::GRAPHICS::VULKAN::CORE {

namespace {

vulkanContext						vk_ctx;

using surfaceCollection = UTL::freeList<vulkanSurface>;
using swapchainCollection = UTL::freeList<vulkanSwapChain>;
surfaceCollection					surfaces;
swapchainCollection					swapchains;

vulkanDescriptorPool						immutable_pool{ descriptorPoolPolicy::NeverFree };										// never update
vulkanDescriptorPool						per_scene_pool{ descriptorPoolPolicy::BulkReset };										// infrequently update
vulkanDescriptorPool						per_frame_pool[frame_buffer_count]{ /* default descriptorPoolPolicy::PerFrameReset*/};	// frequently update / reset the pool in each frame
vulkanDescriptorPool						per_draw_pool{ descriptorPoolPolicy::Linear };											// most frequently update
vulkanDescriptorPool						deferred_pool{ descriptorPoolPolicy::DeferredFree };									// hot reload / deferred free

u32									deferred_releases_flag[frame_buffer_count];
std::mutex							deferred_releases_mutex{};

vulkanCommandPool					command_pool{};

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

void __declspec(noinline) process_deferred_releases() {
	std::lock_guard lock{ deferred_releases_mutex };
	deferred_pool.process_deferred_free();
}

}

namespace DETAIL {

void deferred_release() {
	const u32 frame_idx{ current_frame_index() };
	std::lock_guard lock{ deferred_releases_mutex };
	// TODO
	set_deferred_releases_flag();
}

}

bool initialize() {
	// check Vulkan Runtime
	if (!check_vulkan_runtime()) {
		return false;
	}

	// create instance -> pick physical device -> create logical device
	VKbCall(vk_ctx.initialize(), "::VULKAN:ERROR Failed to initialize Vulkan Context\n");
#ifdef _DEBUG
	vk_ctx.setupDebugMessenger();
#endif

	// create descriptor pools
	VKbCall(immutable_pool.initialize(vk_ctx.device(), 2048, VKX::immutablePoolSizes), "::VULKAN:ERROR Failed to initialize immutable descriptor pool\n");
	VKbCall(per_scene_pool.initialize(vk_ctx.device(), 256, VKX::perScenePoolSizes), "::VULKAN:ERROR Failed to initialize per scene descriptor pool\n");
	for (u32 i{0}; i < frame_buffer_count; ++i) {
		VKbCall(per_frame_pool[i].initialize(vk_ctx.device(), 512, VKX::perFramePoolSizes), "::VULKAN:ERROR Failed to initialize per frame descriptor pool\n");
	}
	VKbCall(per_draw_pool.initialize(vk_ctx.device(), 1024, VKX::perDrawPoolSizes), "::VULKAN:ERROR Failed to initialize per draw descriptor pool\n");
	VKbCall(deferred_pool.initialize(vk_ctx.device(), 512, VKX::deferredPoolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT), "::VULKAN:ERROR Failed to initialize deferred descriptor pool\n");

	// create command pool ... inner error message
	command_pool.initialize(vk_ctx.device_context());

	VKbCall(SHADERS::loadEngineShaders(), "::VULKAN:ERROR Failed to load engine built-in shaders");

	return true;
}

void shutdown() {

	immutable_pool.release();
	per_scene_pool.release();
	for (auto& pool : per_frame_pool) {
		pool.release();
	}
	per_draw_pool.release();
	deferred_pool.release();

	process_deferred_releases();

	vk_ctx.shutdown();
}

VkPhysicalDevice physical_device() {
	return vk_ctx.physical_device();
}

VkDevice device() {
	assert(vk_ctx.device() != VK_NULL_HANDLE);
	return vk_ctx.device();
}

VkQueue graphics_queue() {
	return vk_ctx.graphics_queue().queue();
}

VkQueue present_queue() {
	return vk_ctx.present_queue().queue();
}

u32 current_frame_index() {
	// TODO get current frame index
	return u32_invalid_id;
}

void set_deferred_releases_flag() {
	deferred_releases_flag[current_frame_index()] = 1; // atomic
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
	// waiting GPU
	swapchains.remove(id);
	surfaces[id].destroy(vk_ctx.instance_context());
	surfaces.remove(id);
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
