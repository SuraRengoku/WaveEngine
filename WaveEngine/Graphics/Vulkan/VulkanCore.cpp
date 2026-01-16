#include "VulkanCore.h"
#include "VulkanSurface.h"
#include "VulkanSwapChain.h"
#include "VulkanShader.h"
#include "VulkanCommand.h"
#include "VulkanSync.h"

namespace WAVEENGINE::GRAPHICS::VULKAN::CORE {

namespace {

vulkanContext						vk_ctx;

std::atomic<u32>					current_frame{ 0 };

using surfaceCollection = UTL::freeList<vulkanSurface>;
using swapchainCollection = UTL::freeList<vulkanSwapChain>;
surfaceCollection					surfaces;
swapchainCollection					swapchains;

vulkanDescriptorPool				immutable_pool{ descriptorPoolPolicy::NeverFree };				// never update
vulkanDescriptorPool				per_scene_pool{ descriptorPoolPolicy::BulkReset };				// infrequently update
vulkanDescriptorPool				per_frame_pool[frame_buffer_count] {
	/* default descriptorPoolPolicy::PerFrameReset*/ };												// frequently update / reset the pool in each frame
vulkanDescriptorPool				per_draw_pool{ descriptorPoolPolicy::Linear };					// most frequently update
vulkanDescriptorPool				deferred_pool{ descriptorPoolPolicy::DeferredFree };			// hot reload / deferred free

u32									deferred_releases_flag[frame_buffer_count];
std::mutex							deferred_releases_mutex{};

vulkanCommandPool					graphics_command_pool[frame_buffer_count]{};					// main rendering
vulkanCommandPool					transfer_command_pool{};										// transfer specified (resource load / transfer)
vulkanCommandPool					transient_command_pool{};										// one time command (initialization)

vulkanCommandBuffer begin_single_time_commands(vulkanCommandPool& cmdPool) {
	UTL::vector<vulkanCommandBuffer> cmdBuffers;
	cmdBuffers.emplace_back();

	cmdPool.allocateBuffers(cmdBuffers);
	if (VkResult result = cmdBuffers[0].begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)) {
		debug_error("::VULKAN:ERROR Failed to begin command");
	}
	return std::move(cmdBuffers[0]);
}

void end_single_time_commands(vulkanCommandPool& cmdPool, vulkanCommandBuffer& cmdBuffer) {
	if (VkResult result = cmdBuffer.end()) {
		debug_error("::VULKAN:ERROR Failed to end command");
	}

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = cmdBuffer.Address();

	vk_ctx.device_context()._graphicsQueue.submit(&submitInfo, VK_NULL_HANDLE);
	vk_ctx.device_context()._graphicsQueue.waitIdle();

	UTL::vector<vulkanCommandBuffer> buffers_to_free;
	buffers_to_free.emplace_back(std::move(cmdBuffer));
	cmdPool.freeBuffers(buffers_to_free);
}

frameContext						frames_data[frame_buffer_count];

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
	for (u32 i{0}; i < frame_buffer_count; ++i) {
		graphics_command_pool[i].initialize(vk_ctx.device_context(), nullptr, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	}
	transfer_command_pool.initialize(vk_ctx.device_context());
	transient_command_pool.initialize(vk_ctx.device_context());

	// allocate command buffers for each frame
	for (u32 i{0}; i < frame_buffer_count; ++i) {
		frames_data[i].frame_index = i;
		UTL::vector<vulkanCommandBuffer> cmdBuffers;
		cmdBuffers.emplace_back();
		VKCall(graphics_command_pool[i].allocateBuffers(cmdBuffers), "::VULKAN:ERROR Failed to allocate command buffer for frame {} \n", i);

		frames_data[i].graphics_cmd_buffer = std::move(cmdBuffers[0]);

		// sync objects
		frames_data[i].fence.create(vk_ctx.device_context(), VK_FENCE_CREATE_SIGNALED_BIT, nullptr);
		frames_data[i].image_available_semaphore.create(vk_ctx.device_context());
		frames_data[i].render_finished_semaphore.create(vk_ctx.device_context());
	}

	VKbCall(SHADERS::loadEngineShaders(), "::VULKAN:ERROR Failed to load engine built-in shaders");

	return true;
}

void shutdown() {
	vkDeviceWaitIdle(vk_ctx.device());

	for (u32 i{0}; i < frame_buffer_count; ++i) {
		frames_data[i].image_available_semaphore.destroy();
		frames_data[i].render_finished_semaphore.destroy();
		frames_data[i].fence.destroy();
	}

	process_deferred_releases();

	for (u32 i{0}; i < frame_buffer_count; ++i) {
		graphics_command_pool[i].release();
	}
	transfer_command_pool.release();
	transient_command_pool.release();

	shaders.clear();

	per_draw_pool.release();
	for (auto& pool : per_frame_pool) {
		pool.release();
	}
	per_scene_pool.release();
	deferred_pool.release();
	immutable_pool.release();

	assert(swapchains.size() == 0);
	assert(surfaces.size() == 0);

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
	return current_frame.load(std::memory_order_acquire);
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
	swapchains[id].release(); // normally surface and swapchain share a single id
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
	const u32 frame_idx = current_frame_index();
	frameContext& frame_data = frames_data[frame_idx];

	assert(frame_data.frame_index == frame_idx);

	// wait for completion of current frame
	frame_data.fence.wait(VK_TRUE, UINT64_MAX);

	// acquire swapchain image
	u32 image_index;
	vkAcquireNextImageKHR(vk_ctx.device(), swapchains[id].swapchain(), UINT64_MAX, frame_data.image_available_semaphore, VK_NULL_HANDLE, &image_index);

	frame_data.fence.reset();

	// reset and begin recording
	frame_data.graphics_cmd_buffer.reset();
	frame_data.graphics_cmd_buffer.begin();

	// ✅ 获取当前 swapchain image
    VkImage swapchain_image = swapchains[id].images()[image_index];

    // ✅ 布局转换: UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL
    VkImageMemoryBarrier barrier_to_color{};
    barrier_to_color.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier_to_color.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier_to_color.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier_to_color.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier_to_color.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier_to_color.image = swapchain_image;
    barrier_to_color.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier_to_color.subresourceRange.baseMipLevel = 0;
    barrier_to_color.subresourceRange.levelCount = 1;
    barrier_to_color.subresourceRange.baseArrayLayer = 0;
    barrier_to_color.subresourceRange.layerCount = 1;
    barrier_to_color.srcAccessMask = 0;
    barrier_to_color.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    vkCmdPipelineBarrier(
        frame_data.graphics_cmd_buffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier_to_color
    );

    // TODO: 在这里执行实际渲染(render pass)

    // ✅ 布局转换: COLOR_ATTACHMENT_OPTIMAL -> PRESENT_SRC_KHR
    VkImageMemoryBarrier barrier_to_present{};
    barrier_to_present.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier_to_present.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier_to_present.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier_to_present.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier_to_present.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier_to_present.image = swapchain_image;
    barrier_to_present.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier_to_present.subresourceRange.baseMipLevel = 0;
    barrier_to_present.subresourceRange.levelCount = 1;
    barrier_to_present.subresourceRange.baseArrayLayer = 0;
    barrier_to_present.subresourceRange.layerCount = 1;
    barrier_to_present.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier_to_present.dstAccessMask = 0;

    vkCmdPipelineBarrier(
        frame_data.graphics_cmd_buffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier_to_present
    );

	// TODO: record rendering command

	frame_data.graphics_cmd_buffer.end();

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	vk_ctx.graphics_queue().submit(frame_data, waitStages);

	vk_ctx.present_queue().present(frame_data, swapchains[id].swapchain(), image_index, nullptr);

	current_frame.store((frame_idx + 1) % frame_buffer_count, std::memory_order_release);
}

}
