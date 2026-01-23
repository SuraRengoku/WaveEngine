#pragma once
#include "VulkanCommonHeaders.h"
#include "VulkanContext.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

class vulkanImage {
private:
    VkImage                        _image{ VK_NULL_HANDLE };
    VkDevice                       _device{ VK_NULL_HANDLE };

public:
    vulkanImage() = default;

    DISABLE_COPY(vulkanImage);

    vulkanImage(const deviceContext& dCtx, const VkImageCreateInfo& createInfo) {
        create(dCtx, createInfo);
    }
    vulkanImage(const deviceContext& dCtx, VkImageType type, VkFormat format,
                    VkExtent3D extent, u32 mLevels, u32 aLayers, VkSampleCountFlagBits samples,
                    VkImageTiling tiling, VkSharingMode sMode, VkImageLayout iniLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    const VkImageUsageFlags uFlags = 0, const VkImageCreateFlags cFlags = 0, const void* next = nullptr) {
        create(dCtx, type, format, extent, mLevels, aLayers, samples, tiling, sMode, iniLayout, uFlags, cFlags, next);
    }

    VK_MOVE_CTOR2(vulkanImage, _image, _device)
    VK_MOVE_ASSIGN2(vulkanImage, _image, _device)

    ~vulkanImage() { VK_DESTROY_PTR_BY(vkDestroyImage, _device, _image) }

    [[nodiscard]] VK_DEFINE_PTR_TYPE_OPERATOR(_image)
    [[nodiscard]] VK_DEFINE_ADDRESS_FUNCTION(_image)

    [[nodiscard]] VkImage getImage() const {
        assert(_image != VK_NULL_HANDLE);
        return _image;
    }

    bool create(const deviceContext& dCtx, const VkImageCreateInfo& createInfo);
    bool create(const deviceContext& dCtx, VkImageType type, VkFormat format,
                    VkExtent3D extent, u32 mLevels, u32 aLayers, VkSampleCountFlagBits samples,
                    VkImageTiling tiling, VkSharingMode sMode, VkImageLayout iniLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    const VkImageUsageFlags uFlags = 0, const VkImageCreateFlags cFlags = 0, const void* next = nullptr);

    bool isValid() const noexcept {
        return _image != VK_NULL_HANDLE;
    }

    VkMemoryAllocateInfo memoryAllocateInfo(VkPhysicalDevice physical_device, VkMemoryPropertyFlags desiredMemoryProperties) const;

    VkResult bindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset = 0);
};

class vulkanImageView {
private:
    VkImageView                 _image_view{ VK_NULL_HANDLE };
    VkDevice                    _device{ VK_NULL_HANDLE };

public:
    vulkanImageView() = default;

    DISABLE_COPY(vulkanImageView);

    vulkanImageView(const deviceContext& dCtx, const VkImageViewCreateInfo& createInfo) {
        create(dCtx, createInfo);
    }
    vulkanImageView(const deviceContext& dCtx, VkImage image, VkFormat format,
                    VkImageViewType type, VkComponentMapping components, const VkImageSubresourceRange& range,
                    const VkImageViewCreateFlags flags = 0 , const void* next = nullptr) {
        create(dCtx, image, format, type, components, range, flags, next);
    }

    VK_MOVE_CTOR2(vulkanImageView, _image_view, _device)
    VK_MOVE_ASSIGN2(vulkanImageView, _image_view, _device)

    ~vulkanImageView() {
        destroy();
    }

    [[nodiscard]] VK_DEFINE_PTR_TYPE_OPERATOR(_image_view)
    [[nodiscard]] VK_DEFINE_ADDRESS_FUNCTION(_image_view)

    [[nodiscard]] VkImageView getImageView() const {
        assert(_image_view != VK_NULL_HANDLE);
        return _image_view;
    }

    bool create(const deviceContext& dCtx, const VkImageViewCreateInfo& createInfo);
    bool create(const deviceContext& dCtx, VkImage image, VkFormat format,
                    VkImageViewType type, const VkComponentMapping& components, const VkImageSubresourceRange& range,
                    const VkImageViewCreateFlags flags = 0 , const void* next = nullptr);

    void destroy() noexcept;

    bool isValid() const noexcept {
        return _image_view != VK_NULL_HANDLE;
    }
};

}

