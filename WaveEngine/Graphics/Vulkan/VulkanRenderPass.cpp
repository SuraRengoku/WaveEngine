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

VkResult vulkanRenderPass::create(const VkRenderPassCreateInfo& createInfo) {
    assert(_device != VK_NULL_HANDLE);
    if (VkResult result = vkCreateRenderPass(_device, &createInfo, nullptr, &_renderPass)) {
        debug_output("::VULKAN: Failed to create a render pass");
        return result;
    }
    return VK_SUCCESS;
}

}
