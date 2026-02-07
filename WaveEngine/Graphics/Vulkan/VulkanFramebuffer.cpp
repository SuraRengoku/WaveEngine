#include "VulkanFramebuffer.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

VkResult vulkanFramebuffer::create(const deviceContext& dCtx, const VkFramebufferCreateInfo& createInfo) {
    assert(_framebuffer == VK_NULL_HANDLE && "::VULKAN:ERROR Can not recreate framebuffer\n");
    _device = dCtx._device;

    if (VkResult result = vkCreateFramebuffer(_device, &createInfo, dCtx._allocator, &_framebuffer)) {
        debug_error("::VULKAN:ERROR Failed to create a framebuffer\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanFramebuffer::create(const deviceContext& dCtx, VkRenderPass renderPass, u32 attachmentCount,
    const VkImageView* pAttachments, u32 width, u32 height, u32 layers, const VkFramebufferCreateFlags& flags,
    const void* next) {
    VkFramebufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.flags = flags;
    createInfo.pNext = next;
    createInfo.renderPass = renderPass;
    createInfo.attachmentCount = attachmentCount;
    createInfo.pAttachments = pAttachments;
    createInfo.width = width;
    createInfo.height = height;
    createInfo.layers = layers;

    return create(dCtx, createInfo);
}

void vulkanFramebuffer::destroy() noexcept {
	if (_framebuffer != VK_NULL_HANDLE) {
        VK_DESTROY_PTR_BY(vkDestroyFramebuffer, _device, _framebuffer)
        _device = VK_NULL_HANDLE;
	}
}

void vulkanFramebufferBuilder::destroy(UTL::vector<vulkanFramebuffer>& framebuffers) {
	for (auto& framebuffer : framebuffers) {
		framebuffer.destroy();
	}
	framebuffers.clear();
}

void vulkanFramebufferBuilder::destroy(vulkanFramebuffer& framebuffer) {
	framebuffer.destroy();
}

bool vulkanFramebufferBuilder::build_for_swapchain(const deviceContext& dCtx,
	const vulkanSwapChain& swapchain,
	const vulkanRenderPass& renderPass,
	const vulkanRenderTarget* depthTarget,
	UTL::vector<vulkanFramebuffer>& outFramebuffers) {

	destroy(outFramebuffers);
	const u32 count = swapchain.imageCount();
	outFramebuffers.resize(count);

	for (u32 i{ 0 }; i < count; ++i) {
		UTL::vector<VkImageView> attachments;
		attachments.push_back(swapchain.imageView(i).handle());

		if (depthTarget && depthTarget->hasDepth()) {
			attachments.push_back(depthTarget->depthView().handle());
		}

		VKCall(outFramebuffers[i].create(dCtx,
			renderPass.handle(),
			static_cast<u32>(attachments.size()),
			attachments.data(),
			swapchain.width(),
			swapchain.height(),
			1),
			"::VULKAN:ERROR Failed to create framebuffer\n");
	}

	return true;
}

bool vulkanFramebufferBuilder::build_for_target(const deviceContext& dCtx,
	const vulkanRenderTarget& target,
	const vulkanRenderPass& renderPass,
	vulkanFramebuffer& outFramebuffer) {

	destroy(outFramebuffer);

	UTL::vector<VkImageView> attachments;
	attachments.reserve(target.colorCount() + (target.hasDepth() ? 1 : 0));

	for (u32 i{ 0 }; i < target.colorCount(); ++i) {
		attachments.push_back(target.imageView(i).handle());
	}

	if (target.hasDepth()) {
		attachments.push_back(target.depthView().handle());
	}

	VKCall(outFramebuffer.create(dCtx,
		renderPass.handle(),
		static_cast<u32>(attachments.size()),
		attachments.data(),
		target.width(),
		target.height(),
		1),
		"::VULKAN:ERROR Failed to create framebuffer\n");

	return true;
}

}
