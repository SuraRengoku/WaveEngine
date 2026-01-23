#include "VulkanCommand.h"
#include "VulkanContext.h"
#include "VulkanPipeline.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

////////////////////////////////////////////// VULKAN COMMAND POOL /////////////////////////////////////////////

bool vulkanCommandPool::initialize(VkDevice device, u32 queueFamilyIndex, const void* next,  VkCommandPoolCreateFlags flags) {
    assert(_pool == VK_NULL_HANDLE && "::VULKAN:ERROR: Can not reinitialize the command pool\n");
    _device = device;
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = flags;
    createInfo.queueFamilyIndex = queueFamilyIndex;
    createInfo.pNext = next;

    VKCall(vkCreateCommandPool(_device, &createInfo, nullptr, &_pool), "::VULKAN:ERROR Failed to create a command pool\n");
    return true;
}

bool vulkanCommandPool::initialize(const deviceContext& dCtx, const void* next, VkCommandPoolCreateFlags flags) {
    assert(_pool == VK_NULL_HANDLE && "::VULKAN:ERROR: Can not reinitialize the command pool\n");
    _device = dCtx._device;
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.pNext = next;
    createInfo.queueFamilyIndex = dCtx._graphicsQueue.familyIndex();
    createInfo.flags = flags;

    VKCall(vkCreateCommandPool(_device, &createInfo, dCtx._allocator, &_pool), "::VULKAN:ERROR Failed to create a command pool\n");
    return true;
}

VkResult vulkanCommandPool::allocateBuffers(UTL::vector<VkCommandBuffer>& buffers, VkCommandBufferLevel level) const {
    assert(_device != VK_NULL_HANDLE && _pool != VK_NULL_HANDLE);
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = _pool;
    allocateInfo.commandBufferCount = buffers.size();
    allocateInfo.level = level;
    if (VkResult result = vkAllocateCommandBuffers(_device, &allocateInfo, buffers.data())) {
        debug_error("::VULKAN:ERROR Failed to allocate command buffers\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanCommandPool::allocateBuffers(UTL::vector<vulkanCommandBuffer>& buffers, VkCommandBufferLevel level) const {
    assert(_device != VK_NULL_HANDLE && _pool != VK_NULL_HANDLE);
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = _pool;
    allocateInfo.commandBufferCount = buffers.size();
    allocateInfo.level = level;
    if (VkResult result = vkAllocateCommandBuffers(_device, &allocateInfo, reinterpret_cast<VkCommandBuffer*>(buffers.data()))) {
        debug_error("::VULKAN:ERROR Failed to allocate command buffers\n");
        return result;
    }
    return VK_SUCCESS;
}

void vulkanCommandPool::freeBuffers(UTL::vector<VkCommandBuffer>& buffers) const {
    if (buffers.empty()) {
        debug_error(":VULKAN:WARNING the container of command buffer is empty\n");
        return;
    }
    vkFreeCommandBuffers(_device, _pool, buffers.size(), buffers.data());
    memset(buffers.data(), 0, buffers.size() * sizeof(VkCommandBuffer));
}

void vulkanCommandPool::freeBuffers(UTL::vector<vulkanCommandBuffer>& buffers) const {
    if (buffers.empty()) {
        debug_error(":VULKAN:WARNING the container of command buffer is empty\n");
        return;
    }
    // TODO if we use ArrayRef we can just implicitly transfer vulkanCommandBuffer to VkCommandBuffer
    // TODO make sure vulkanCommandBuffer only has one element and no virtual functions
    static_assert(sizeof(vulkanCommandBuffer) == sizeof(VkCommandBuffer),
        "::VULKAN:ERROR vulkanCommandBuffer must have the same size as VkCommandBuffer\n");

    vkFreeCommandBuffers(_device, _pool, buffers.size(), reinterpret_cast<const VkCommandBuffer*>(buffers.data()));
    memset(buffers.data(), 0, buffers.size() * sizeof(vulkanCommandBuffer));
}

void vulkanCommandPool::release() {
    assert(_pool != VK_NULL_HANDLE);
    VK_DESTROY_PTR_BY(vkDestroyCommandPool, _device, _pool);
}

////////////////////////////////////////////// VULKAN COMMAND BUFFER /////////////////////////////////////////////

VkResult vulkanCommandBuffer::beginCmd(VkCommandBufferUsageFlags usageFlags,
                                    const VkCommandBufferInheritanceInfo& inheritanceInfo) const {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = usageFlags;
    beginInfo.pInheritanceInfo = &inheritanceInfo;
    if (VkResult result = vkBeginCommandBuffer(_commandBuffer, &beginInfo)) {
        debug_error("::VULKAN:ERROR Failed to begin a command buffer\n");
        return result;
    }
    return VK_SUCCESS;
}

VkResult vulkanCommandBuffer::beginCmd(VkCommandBufferUsageFlags usageFlags) const {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = usageFlags;
    beginInfo.pInheritanceInfo = VK_NULL_HANDLE;
    if (VkResult result = vkBeginCommandBuffer(_commandBuffer, &beginInfo)) {
        debug_error("::VULKAN:ERROR Failed to begin a command buffer\n");
        return result;
    }
    return VK_SUCCESS;
}

void vulkanCommandBuffer::resetCmd() const {
    vkResetCommandBuffer(_commandBuffer, 0);
}

VkResult vulkanCommandBuffer::endCmd() const {
    if (VkResult result = vkEndCommandBuffer(_commandBuffer)) {
        debug_error("::VULKAN:ERROR Failed to end a command buffer\n");
        return result;
    }
    return VK_SUCCESS;
}

////////////////////////////////////////////// VULKAN RENDER ENCODER ///////////////////////////////////////////


vulkanRenderEncoder::vulkanRenderEncoder(const vulkanCommandBuffer& cmd, VkRenderPass renderPass,
    VkFramebuffer framebuffer) : _commandBuffer(cmd.handle()), _renderPass(renderPass), _framebuffer(framebuffer) {
    assert(_commandBuffer != VK_NULL_HANDLE);
    assert(_renderPass != VK_NULL_HANDLE);
    assert(_framebuffer != VK_NULL_HANDLE);
}



void vulkanRenderEncoder::beginRender(const VkRect2D& renderArea, const VkClearValue* clearValues, u32 clearCount, 
                                      VkSubpassContents contents) {
    assert(!_active);

    VkRenderPassBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.renderPass = _renderPass;
    beginInfo.framebuffer = _framebuffer;
    beginInfo.renderArea = renderArea;
    beginInfo.clearValueCount = clearCount;
    beginInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(_commandBuffer, &beginInfo, contents);
    _active = true;
}

void vulkanRenderEncoder::nextSubpass(VkSubpassContents contents) {
    assert(_active);
    vkCmdNextSubpass(_commandBuffer, contents);
}

void vulkanRenderEncoder::endRender() {
    assert(_active);
    vkCmdEndRenderPass(_commandBuffer);
    _active = false;
}

void vulkanRenderEncoder::setViewport(const VkViewport& viewport) {
    assert(_active);
    vkCmdSetViewport(_commandBuffer, 0, 1, &viewport);
}
void vulkanRenderEncoder::setViewport(float x, float y, float width, float height, float maxDepth, float minDepth) {
    VkViewport viewport{ x, y, width, height, minDepth, maxDepth };
    setViewport(viewport);
}

void vulkanRenderEncoder::setScissor(const VkRect2D& scissor) {
    assert(_active);
    vkCmdSetScissor(_commandBuffer, 0, 1, &scissor);
}

void vulkanRenderEncoder::bindPipeline(const vulkanPipeline& pipeline) {
    assert(_active);
    vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline());
}

void vulkanRenderEncoder::bindDescriptorSets(VkPipelineBindPoint bindPoint, VkPipelineLayout layout, u32 firstSet,
    const VkDescriptorSet* sets, u32 setCount, const u32* dynamicOffsets, u32 dynamicOffsetCount) {
    assert(_active);
    vkCmdBindDescriptorSets(_commandBuffer, bindPoint, layout, firstSet, setCount, sets, dynamicOffsetCount, dynamicOffsets);
}

void vulkanRenderEncoder::bindVertexBuffers(u32 firstBinding, u32 bindingCount, const VkBuffer* buffers,
    const VkDeviceSize* offsets) {
    assert(_active);
    vkCmdBindVertexBuffers(_commandBuffer, firstBinding, bindingCount, buffers, offsets);
}

void vulkanRenderEncoder::bindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) {
    assert(_active);
    vkCmdBindIndexBuffer(_commandBuffer, buffer, offset, indexType);
}

void vulkanRenderEncoder::draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance) {
    assert(_active);
    vkCmdDraw(_commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void vulkanRenderEncoder::drawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex, int32_t vertexOffset,
    u32 firstInstance) {
    assert(_active);
    vkCmdDrawIndexed(_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void vulkanRenderEncoder::reset() {
    assert(!_active && "Cannot reset active render encoder");
    _commandBuffer = VK_NULL_HANDLE;
    _renderPass = VK_NULL_HANDLE;
    _framebuffer = VK_NULL_HANDLE;
}

bool vulkanRenderEncoder::needsReinit(VkRenderPass renderPass, VkFramebuffer framebuffer) const {
#ifdef _DEBUG
    bool rpChanged = _renderPass != renderPass;
    bool fbChanged = _framebuffer != framebuffer;
    if (rpChanged || fbChanged) {
        debug_output("::VULKAN:INFO Encoder needs reinit - RenderPass changed: %d, Framebuffer changed: %d\n",
            rpChanged, fbChanged);
    }
#endif
    return _renderPass != renderPass || _framebuffer != framebuffer;
}

}
