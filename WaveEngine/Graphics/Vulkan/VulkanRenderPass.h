#pragma once
#include "VulkanCommonHeaders.h"
#include "VulkanContext.h"
#include "VulkanPipeline.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

/* ===================================== RenderPassDesc.h ==================================== */
// TODO this should be moved to a higher abstract level (namespace RENDER)
struct renderPassDesc {
	struct attachment {
        VkFormat                format;
        VkSampleCountFlagBits   samples = VK_SAMPLE_COUNT_1_BIT;

        VkAttachmentLoadOp      loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        VkAttachmentStoreOp     storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        VkAttachmentLoadOp      depthStencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        VkAttachmentStoreOp     depthStencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        VkImageLayout           initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageLayout           finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	};

    struct subpass {
        UTL::vector<u32>        colorAttachments;
        std::optional<u32>      depthAttachment;
        std::optional<u32>      resolveAttachment;
        UTL::vector<u32>        inputAttachments;
    };

    UTL::vector<attachment>                         attachments;
    UTL::vector<subpass>                            subpasses;
    UTL::vector<VkSubpassDependency>                dependencies;

    std::optional<VkRenderPassMultiviewCreateInfo>  multiview;
};

/* =================================== RenderPassFactory.h =================================== */
// TODO this should be moved to a higher abstract level (namespace RENDER)
struct GBufferFormats {
    VkFormat            albedo;
    VkFormat            normal;
    VkFormat            material;
};

class renderPassFactory {
public:
    static renderPassDesc Shadow(VkFormat depthFormat);
    static renderPassDesc Forward(VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits samples, bool hdr);
    static renderPassDesc GBuffer(const GBufferFormats& colorFormat, VkFormat depthFormat, VkSampleCountFlagBits samples);
    static renderPassDesc Lighting(const GBufferFormats& colorFormats, VkFormat outputFormat, bool hdr = true);
    static renderPassDesc PostProcess(VkFormat inputFormat, VkFormat outputFormat, bool hdr);
};

/* =================================== RenderPassBuilder.h =================================== */
struct renderPassScratch {
    UTL::vector<VkAttachmentDescription>            attachments;
    UTL::vector<VkSubpassDescription>               subpasses;
    UTL::vector<UTL::vector<VkAttachmentReference>> colorRefs;
    UTL::vector<UTL::vector<VkAttachmentReference>> inputRefs;
    UTL::vector<VkAttachmentReference>              depthRefs;
    UTL::vector<VkAttachmentReference>              resolveRefs;
};

class renderPassBuilder {
public:
    static VkRenderPassCreateInfo build(
        const renderPassDesc& desc,
		renderPassScratch& scratch);

    static VkRenderPassCreateInfo build(const renderPassDesc& desc,
        UTL::vector<VkAttachmentDescription>& outAttachments, UTL::vector<VkSubpassDescription>& outSubpasses,
        UTL::vector<UTL::vector<VkAttachmentReference>>& outColorRefs, UTL::vector<UTL::vector<VkAttachmentReference>>& outInputRefs,
        UTL::vector<VkAttachmentReference>& outDepthRefs, UTL::vector<VkAttachmentReference>& outResolveRefs);
};

/* ==================================== VulkanRenderPass.h =================================== */
class vulkanRenderPass {
private:
    VkRenderPass            _renderPass{ VK_NULL_HANDLE };
    VkDevice                _device{ VK_NULL_HANDLE };

public:
    vulkanRenderPass() = default;

    DISABLE_COPY(vulkanRenderPass)

    VK_MOVE_CTOR2(vulkanRenderPass, _renderPass, _device)
    VK_MOVE_ASSIGN2(vulkanRenderPass, _renderPass, _device)

    ~vulkanRenderPass() {
        destroy();
    }

    void destroy() noexcept;

    [[nodiscard]] VK_DEFINE_PTR_TYPE_OPERATOR(_renderPass)
    [[nodiscard]] VK_DEFINE_ADDRESS_FUNCTION(_renderPass)

    [[nodiscard]] const VkRenderPass& handle() const {
        assert(_renderPass != VK_NULL_HANDLE);
        return _renderPass;
    }

    bool isValid() const noexcept { return _renderPass != VK_NULL_HANDLE; }

    VkResult createInternal(const deviceContext& dCtx, const VkRenderPassCreateInfo& createInfo);

    /* ============================== Specified Creations ================================== */
    // TODO these functions should be moved to a higher abstract level 

    VkResult createShadow(const deviceContext& dCtx, VkFormat depthFmt);

    VkResult createForward(const deviceContext& dCtx, VkFormat colorFmt, VkFormat depthFmt, VkSampleCountFlagBits samples, bool hdr);

    VkResult createGBuffer(const deviceContext& dCtx, const GBufferFormats& colorFmts, VkFormat depthFmt, VkSampleCountFlagBits samples);

    VkResult createLighting(const deviceContext& dCtx, const GBufferFormats& colorFmts, VkFormat outputFmt, bool hdr);

    VkResult createPostProcess(const deviceContext& dCtx, VkFormat inputFmt, VkFormat outputFmt, bool hdr);

    /* ================================ Command Helpers ==================================== */
    // Removed 
};

}