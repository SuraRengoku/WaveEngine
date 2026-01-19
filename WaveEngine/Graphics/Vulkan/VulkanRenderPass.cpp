#include "VulkanRenderPass.h"

namespace  WAVEENGINE::GRAPHICS::VULKAN {

VkRenderPassCreateInfo renderPassBuilder::build(const renderPassDesc& desc, renderPassScratch& scratch) {
    return build(desc, scratch.attachments, scratch.subpasses, scratch.colorRefs, scratch.inputRefs, scratch.depthRefs, scratch.resolveRefs);
}

VkRenderPassCreateInfo renderPassBuilder::build(const renderPassDesc& desc,
    UTL::vector<VkAttachmentDescription>& outAttachments, UTL::vector<VkSubpassDescription>& outSubpasses,
    UTL::vector<UTL::vector<VkAttachmentReference>>& outColorRefs, UTL::vector<UTL::vector<VkAttachmentReference>>& outInputRefs,
    UTL::vector<VkAttachmentReference>& outDepthRefs, UTL::vector<VkAttachmentReference>& outResolveRefs) {

    // attachments
    outAttachments.reserve(desc.attachments.size());
    for (auto&  rpaDesc: desc.attachments) {
        VkAttachmentDescription attachment{};
        attachment.format = rpaDesc.format;
        attachment.samples = rpaDesc.samples;
        attachment.loadOp = rpaDesc.loadOp;
        attachment.storeOp = rpaDesc.storeOp;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = rpaDesc.initialLayout;
        attachment.finalLayout = rpaDesc.finalLayout;
        outAttachments.push_back(attachment);
    }

    // subpasses
    outColorRefs.resize(desc.subpasses.size());
    outInputRefs.resize(desc.subpasses.size());
    outSubpasses.resize(desc.subpasses.size());
    outDepthRefs.reserve(desc.subpasses.size());
    outResolveRefs.reserve(desc.subpasses.size());

    for (u32 i{0}; i < desc.subpasses.size(); ++i) {
        auto& sp = desc.subpasses[i];
        auto& vksp = outSubpasses[i];

        assert(sp.depthAttachment < static_cast<int>(desc.attachments.size()));

        for (u32 idx : sp.colorAttachments) {
            assert(idx < desc.attachments.size());
            outColorRefs[i].push_back({ idx, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
        }
        for (u32 idx: sp.inputAttachments) {
            assert(idx < desc.attachments.size());
            outInputRefs[i].push_back({ idx, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
        }

        vksp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        vksp.colorAttachmentCount = static_cast<u32>(outColorRefs[i].size());
        vksp.pColorAttachments = outColorRefs[i].data();
        vksp.inputAttachmentCount = static_cast<u32>(outInputRefs[i].size());
        vksp.pInputAttachments = outInputRefs[i].data();

        if (sp.depthAttachment.has_value()) {
            outDepthRefs.push_back({ sp.depthAttachment.value(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL });
            vksp.pDepthStencilAttachment = &outDepthRefs.back();
        }

        if (sp.resolveAttachment.has_value()) {
            outResolveRefs.push_back({ sp.resolveAttachment.value(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
            vksp.pResolveAttachments = &outResolveRefs.back();
        }
    }

    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = static_cast<u32>(outAttachments.size());
    createInfo.pAttachments = outAttachments.data();
    createInfo.subpassCount = static_cast<u32>(outSubpasses.size());
    createInfo.pSubpasses = outSubpasses.data();
    createInfo.dependencyCount = static_cast<u32>(desc.dependencies.size());
    createInfo.pDependencies = desc.dependencies.data();

    return createInfo;
}

renderPassDesc renderPassFactory::Shadow(VkFormat depthFormat) {
    renderPassDesc desc{};

    // Depth-only attachment
    desc.attachments = {
        {
            depthFormat, 
        	VK_SAMPLE_COUNT_1_BIT, 
        	VK_ATTACHMENT_LOAD_OP_CLEAR, 
        	VK_ATTACHMENT_STORE_OP_STORE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        }
    };

    // Single subpass with depth attachment only
    desc.subpasses = { {{}, 0} };

    return desc;
}

renderPassDesc renderPassFactory::Forward(VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits samples,
    bool hdr) {
    renderPassDesc desc{};

    desc.attachments = {
        {
            colorFormat, 
        	samples, 
        	VK_ATTACHMENT_LOAD_OP_CLEAR, 
        	VK_ATTACHMENT_STORE_OP_STORE,
        	VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            hdr ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        },
        {
            depthFormat, 
        	samples, 
        	VK_ATTACHMENT_LOAD_OP_CLEAR, 
        	VK_ATTACHMENT_STORE_OP_DONT_CARE,
        	VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        }
    };

    desc.subpasses = { {{0}, 1} };

    return desc;
}

renderPassDesc renderPassFactory::GBuffer(const GBufferFormats& colorFormat, VkFormat depthFormat,
    VkSampleCountFlagBits samples) {
    renderPassDesc desc{};

    // GBuffer attachments: albedo, normal, material (all saved for shader read), depth
    desc.attachments = {
        {
            colorFormat.albedo, 
        	samples, 
        	VK_ATTACHMENT_LOAD_OP_CLEAR, 
        	VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        },
        {
            colorFormat.normal, 
        	samples, 
        	VK_ATTACHMENT_LOAD_OP_CLEAR,
        	VK_ATTACHMENT_STORE_OP_STORE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        },
        {
            colorFormat.material, 
        	samples, 
        	VK_ATTACHMENT_LOAD_OP_CLEAR, 
        	VK_ATTACHMENT_STORE_OP_STORE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        },
        {
            depthFormat, 
        	samples, 
        	VK_ATTACHMENT_LOAD_OP_CLEAR, 
        	VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
        }
    };

    // Single subpass with 3 color attachments and depth
    desc.subpasses = { {{0, 1, 2}, 3} };

    return desc;
}

renderPassDesc renderPassFactory::Lighting(const GBufferFormats& colorFormats, VkFormat outputFormat, bool hdr) {
    renderPassDesc desc{};

    // GBuffer input attachments (albedo, normal, material) - load existing data
    desc.attachments = {
        {
            colorFormats.albedo, 
        	VK_SAMPLE_COUNT_1_BIT, 
        	VK_ATTACHMENT_LOAD_OP_LOAD, 
        	VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        },
        {
            colorFormats.normal, 
        	VK_SAMPLE_COUNT_1_BIT, 
        	VK_ATTACHMENT_LOAD_OP_LOAD, 
        	VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        },
        {
            colorFormats.material,
        	VK_SAMPLE_COUNT_1_BIT, 
        	VK_ATTACHMENT_LOAD_OP_LOAD, 
        	VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        },
        // Output color attachment
        {
            outputFormat,
        	VK_SAMPLE_COUNT_1_BIT, 
        	VK_ATTACHMENT_LOAD_OP_CLEAR, 
        	VK_ATTACHMENT_STORE_OP_STORE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            hdr ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        }
    };

    // Subpass: output to attachment 3, read from input attachments 0, 1, 2
    renderPassDesc::subpass subpass{};
    subpass.colorAttachments = {3};
    subpass.inputAttachments = {0, 1, 2};
    desc.subpasses = {subpass};

    // Dependency: ensure GBuffer write completes before lighting read
    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dep.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    desc.dependencies = {dep};

    return desc;
}

renderPassDesc renderPassFactory::PostProcess(VkFormat inputFormat, VkFormat outputFormat, bool hdr) {
    renderPassDesc desc{};

    desc.attachments = {
        // Input attachment - load existing data
        {
            inputFormat, 
        	VK_SAMPLE_COUNT_1_BIT,
        	VK_ATTACHMENT_LOAD_OP_LOAD, 
        	VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        },
        // Output attachment
        {
            outputFormat, 
        	VK_SAMPLE_COUNT_1_BIT, 
        	VK_ATTACHMENT_LOAD_OP_CLEAR, 
        	VK_ATTACHMENT_STORE_OP_STORE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            hdr ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        }
    };

    // Subpass: output to attachment 1, read from input attachment 0
    renderPassDesc::subpass subpass{};
    subpass.colorAttachments = {1};
    subpass.inputAttachments = {0};
    desc.subpasses = {subpass};

    // Dependency: ensure previous pass completes before post-process
    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dep.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    desc.dependencies = {dep};

    return desc;
}

VkResult vulkanRenderPass::createInternal(const deviceContext& dCtx, const VkRenderPassCreateInfo& createInfo) {
    assert(_renderPass == VK_NULL_HANDLE);
    _device = dCtx._device;

    if (VkResult result = vkCreateRenderPass(_device, &createInfo, dCtx._allocator, &_renderPass)) {
        debug_error("::VULKAN:ERROR Failed to create a render pass\n");
        return result;
    }
    return VK_SUCCESS;
}

void vulkanRenderPass::destroy() noexcept {
    if (_renderPass != VK_NULL_HANDLE) {
        VK_DESTROY_PTR_BY(vkDestroyRenderPass, _device, _renderPass);
        _device = VK_NULL_HANDLE;
    }
}

VkResult vulkanRenderPass::createShadow(const deviceContext& dCtx, VkFormat depthFmt) {
    renderPassDesc desc = renderPassFactory::Shadow(depthFmt);

    UTL::vector<VkAttachmentDescription> attachments;
    UTL::vector<VkSubpassDescription> subpasses;
    UTL::vector<UTL::vector<VkAttachmentReference>> colorRefs;
    UTL::vector<UTL::vector<VkAttachmentReference>> inputRefs;
    UTL::vector<VkAttachmentReference> depthRefs;
    UTL::vector<VkAttachmentReference> resolveRefs;

    VkRenderPassCreateInfo createInfo = renderPassBuilder::build(desc, attachments, subpasses, colorRefs, inputRefs, depthRefs, resolveRefs);

    return createInternal(dCtx, createInfo);
}

// swap chain Format, depthFormat, VK_SAMPLE_COUNT_4_BIT, true
VkResult vulkanRenderPass::createForward(const deviceContext& dCtx, VkFormat colorFmt, VkFormat depthFmt, VkSampleCountFlagBits samples, bool hdr) {
    renderPassDesc desc = renderPassFactory::Forward(colorFmt, depthFmt, samples, hdr);

    UTL::vector<VkAttachmentDescription> attachments;
    UTL::vector<VkSubpassDescription> subpasses;
    UTL::vector<UTL::vector<VkAttachmentReference>> colorRefs;
    UTL::vector<UTL::vector<VkAttachmentReference>> inputRefs;
    UTL::vector<VkAttachmentReference> depthRefs;
    UTL::vector<VkAttachmentReference> resolveRefs;

    VkRenderPassCreateInfo createInfo = renderPassBuilder::build(desc, attachments, subpasses, colorRefs, inputRefs, depthRefs, resolveRefs);

    return createInternal(dCtx, createInfo);
}

VkResult vulkanRenderPass::createGBuffer(const deviceContext& dCtx, const GBufferFormats& colorFmts, VkFormat depthFmt, VkSampleCountFlagBits samples) {
    renderPassDesc desc = renderPassFactory::GBuffer(colorFmts, depthFmt, samples);

    UTL::vector<VkAttachmentDescription> attachments;
    UTL::vector<VkSubpassDescription> subpasses;
    UTL::vector<UTL::vector<VkAttachmentReference>> colorRefs;
    UTL::vector<UTL::vector<VkAttachmentReference>> inputRefs;
    UTL::vector<VkAttachmentReference> depthRefs;
    UTL::vector<VkAttachmentReference> resolveRefs;

    VkRenderPassCreateInfo createInfo = renderPassBuilder::build(desc, attachments, subpasses, colorRefs, inputRefs, depthRefs, resolveRefs);

    return createInternal(dCtx, createInfo);
}

VkResult vulkanRenderPass::createLighting(const deviceContext& dCtx, const GBufferFormats& colorFmts, VkFormat outputFmt, bool hdr) {
    renderPassDesc desc = renderPassFactory::Lighting(colorFmts, outputFmt, hdr);

    UTL::vector<VkAttachmentDescription> attachments;
    UTL::vector<VkSubpassDescription> subpasses;
    UTL::vector<UTL::vector<VkAttachmentReference>> colorRefs;
    UTL::vector<UTL::vector<VkAttachmentReference>> inputRefs;
    UTL::vector<VkAttachmentReference> depthRefs;
    UTL::vector<VkAttachmentReference> resolveRefs;

    VkRenderPassCreateInfo createInfo = renderPassBuilder::build(desc, attachments, subpasses, colorRefs, inputRefs, depthRefs, resolveRefs);

    return createInternal(dCtx, createInfo);
}

VkResult vulkanRenderPass::createPostProcess(const deviceContext& dCtx, VkFormat inputFmt, VkFormat outputFmt, bool hdr) {
    renderPassDesc desc = renderPassFactory::PostProcess(inputFmt, outputFmt, hdr);

    UTL::vector<VkAttachmentDescription> attachments;
    UTL::vector<VkSubpassDescription> subpasses;
    UTL::vector<UTL::vector<VkAttachmentReference>> colorRefs;
    UTL::vector<UTL::vector<VkAttachmentReference>> inputRefs;
    UTL::vector<VkAttachmentReference> depthRefs;
    UTL::vector<VkAttachmentReference> resolveRefs;

    VkRenderPassCreateInfo createInfo = renderPassBuilder::build(desc, attachments, subpasses, colorRefs, inputRefs, depthRefs, resolveRefs);

    return createInternal(dCtx, createInfo);
}

}
