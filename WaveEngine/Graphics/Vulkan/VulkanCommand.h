#pragma once
#include "VulkanCommonHeaders.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

struct deviceContext;

class vulkanPipeline;

class vulkanCommandBuffer;

// for better performance, we should allocate different command pools to each thread to avoid accessing conflicts
class vulkanCommandPool {
private:
    VkCommandPool                       _pool{ VK_NULL_HANDLE };
    VkDevice                            _device{ VK_NULL_HANDLE };

public:
    vulkanCommandPool() = default;

    DISABLE_COPY(vulkanCommandPool)

    VK_MOVE_CTOR1(vulkanCommandPool, _pool)
    VK_MOVE_ASSIGN1(vulkanCommandPool, _pool)

    ~vulkanCommandPool() {
        assert(_pool == VK_NULL_HANDLE);
        _device = VK_NULL_HANDLE;
    }

    [[nodiscard]] VK_DEFINE_PTR_TYPE_OPERATOR(_pool)
    [[nodiscard]] VK_DEFINE_ADDRESS_FUNCTION(_pool)

    bool initialize(VkDevice device, u32 queueFamilyIndex, const void* next = nullptr, VkCommandPoolCreateFlags flags = 0);
    bool initialize(const deviceContext& dCtx, const void* next = nullptr, VkCommandPoolCreateFlags flags = 0);

    VkResult allocateBuffers(UTL::vector<VkCommandBuffer>& buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const;
    VkResult allocateBuffers(UTL::vector<vulkanCommandBuffer>& buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const;

    void freeBuffers(UTL::vector<VkCommandBuffer>& buffers) const;
    void freeBuffers(UTL::vector<vulkanCommandBuffer>& buffers) const;

    void release();
};

class vulkanCommandBuffer {
private:
    friend class vulkanCommandPool; // to let the pool access private field _commandBuffer
#if _DEBUG
    // vulkanCommandPool*                  container{ nullptr };
#endif
    VkCommandBuffer                     _commandBuffer{ VK_NULL_HANDLE };

public:
    vulkanCommandBuffer() = default;

    DISABLE_COPY(vulkanCommandBuffer)

    VK_MOVE_CTOR1(vulkanCommandBuffer, _commandBuffer)
    VK_MOVE_ASSIGN1(vulkanCommandBuffer, _commandBuffer)

    // no destructor because the function releasing CommandBuffer has been defined in vulkanCommandPool
    VK_DEFINE_PTR_TYPE_OPERATOR(_commandBuffer)
    VK_DEFINE_ADDRESS_FUNCTION(_commandBuffer)

	[[nodiscard]] const VkCommandBuffer& handle() const noexcept {
        return _commandBuffer;
    }

    // begin recording
    [[nodiscard]] VkResult beginCmd(VkCommandBufferUsageFlags usageFlags, const VkCommandBufferInheritanceInfo& inheritanceInfo) const;
    [[nodiscard]] VkResult beginCmd(VkCommandBufferUsageFlags usageFlags = 0) const;

    void resetCmd() const;

    // finish recording
    [[nodiscard]] VkResult endCmd() const;

    bool isValid() const noexcept {
        return _commandBuffer != VK_NULL_HANDLE;
    }
};

class vulkanRenderEncoder {
public:
    vulkanRenderEncoder() = default; // for optional

    vulkanRenderEncoder(const vulkanCommandBuffer& cmd,
        VkRenderPass renderPass, VkFramebuffer framebuffer);

    // ================= RenderPass Scope ===================
    void beginRender(const VkRect2D& renderArea, const VkClearValue* clearValues, u32 clearCount, 
        VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);

    void nextSubpass(VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);

    void endRender();

    // =================== Dynamic state ====================
    void setViewport(const VkViewport& viewport);
    void setViewport(float x, float y, float width, float height, float maxDepth, float minDepth);
    void setScissor(const VkRect2D& scissor);

    // ================ Pipeline & Resources ================
    void bindPipeline(const vulkanPipeline& pipeline);

    void bindDescriptorSets(VkPipelineBindPoint bindPoint, VkPipelineLayout layout,
        u32 firstSet, const VkDescriptorSet* sets, u32 setCount, 
        const u32* dynamicOffsets, u32 dynamicOffsetCount);

    void bindVertexBuffers(u32 firstBinding, u32 bindingCount, 
        const VkBuffer* buffers, const VkDeviceSize* offsets);

    void bindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType);

    // ===================== Draw Calls =====================
    void draw(u32 vertexCount, u32 instanceCount = 1, u32 firstVertex = 0, u32 firstInstance = 0);
    
    void drawIndexed(u32 indexCount, u32 instanceCount = 1, u32 firstIndex = 0, 
        int32_t vertexOffset = 0, u32 firstInstance = 0);

    // ======================== Utils =======================
    void reset();
	
    const VkCommandBuffer& commandBuffer() const noexcept {
        assert(_commandBuffer != VK_NULL_HANDLE);
        return _commandBuffer;
    }

    const VkRenderPass& renderPass() const noexcept {
        assert(_renderPass != VK_NULL_HANDLE);
        return _renderPass;
    }

    const VkFramebuffer& framebuffer() const noexcept {
	    assert(_framebuffer != VK_NULL_HANDLE);
        return _framebuffer;
    }

    bool needsReinit(VkRenderPass renderPass, VkFramebuffer framebuffer) const;

    // ================ use cached encoder ===================


//	const vulkanFramebuffer& fb_wrapper = swapchains[id].framebuffer(image_index);
//	VkFramebuffer current_framebuffer = fb_wrapper.handle();
//
//#ifdef _DEBUG
//	static VkFramebuffer last_framebuffers[frame_buffer_count] = {};
//	if (last_framebuffers[image_index] != current_framebuffer) {
//		debug_output("::VULKAN:WARNING Framebuffer for image %u changed: %p -> %p\n",
//			image_index, (void*)last_framebuffers[image_index], (void*)current_framebuffer);
//		last_framebuffers[image_index] = current_framebuffer;
//	}
//#endif
//
//	auto& encoder_opt = frame_data.render_encoders[image_index];
//
//	if (!encoder_opt.has_value()
//		|| encoder_opt->needsReinit(render_passes.forward, current_framebuffer)) {
//#ifdef _DEBUG
//		debug_output("::VULKAN:INFO Reinitialize encoder for swapchain image %u, framebuffer %p\n",
//			image_index, (void*)current_framebuffer);
//#endif
//		encoder_opt.emplace(
//			frame_data.graphics_cmd_buffer,
//			render_passes.forward,
//			current_framebuffer);
//	}
//
//	auto& render_encoder = encoder_opt.value();
    //auto& render_encoder = frame_data.render_encoders[image_index].value();
    // =======================================================

private:
    VkCommandBuffer                 _commandBuffer{ VK_NULL_HANDLE};
    VkRenderPass                    _renderPass{ VK_NULL_HANDLE };
    VkFramebuffer                   _framebuffer{ VK_NULL_HANDLE };
    bool                            _active{ false };
};

}
