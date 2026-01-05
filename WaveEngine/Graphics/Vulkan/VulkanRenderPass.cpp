#include "VulkanRenderPass.h"

namespace  WAVEENGINE::GRAPHICS::VULKAN {

void vulkanRenderPass::cmdBegin(VkCommandBuffer commandBuffer, VkRenderPassBeginInfo& beginInfo,
    VkSubpassContents subpassContents) const {
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.renderPass = _renderPass;
    vkCmdBeginRenderPass(commandBuffer, &beginInfo, subpassContents);
}

void vulkanRenderPass::cmdBegin(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, VkRect2D renderArea,
    UTL::vector<const VkClearValue> clearValues, VkSubpassContents subpassContents) const {
    VkRenderPassBeginInfo beginInfo{};
    beginInfo.framebuffer = framebuffer;
    beginInfo.clearValueCount = clearValues.size();
    beginInfo.pClearValues = clearValues.data();
    beginInfo.renderArea = renderArea;
    cmdBegin(commandBuffer, beginInfo, subpassContents);
}

VkResult vulkanRenderPass::create(const deviceContext& dCtx, const VkRenderPassCreateInfo& createInfo) {
    assert(_renderPass == VK_NULL_HANDLE && "::VULKAN:ERROR Can not recreate render pass\n");
    _device = dCtx._device;

    if (VkResult result = vkCreateRenderPass(_device, &createInfo, dCtx._allocator, &_renderPass)) {
        debug_error("::VULKAN:ERROR Failed to create a render pass\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanRenderPass::create(const deviceContext& dCtx, u32 attachmentCount,
    const VkAttachmentDescription* pAttachments, u32 subpassCount, const VkSubpassDescription* pSubpass,
    u32 dependencyCount, const VkSubpassDependency* pDependencies, const VkRenderPassCreateFlags& flags,
    const void* next) {
    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.flags = flags;
    createInfo.pNext = next;
    createInfo.attachmentCount = attachmentCount;
    createInfo.pAttachments = pAttachments;
    createInfo.subpassCount = subpassCount;
    createInfo.pSubpasses = pSubpass;
    createInfo.dependencyCount = dependencyCount;
    createInfo.pDependencies = pDependencies;

    return create(dCtx, createInfo);
}

}
