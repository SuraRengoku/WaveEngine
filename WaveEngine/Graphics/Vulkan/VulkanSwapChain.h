#pragma once
#include "VulkanCommonHeaders.h"
#include "VulkanImage.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

DEFINE_TYPED_ID(swapchain_id);

namespace DETAIL {

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    UTL::vector<VkSurfaceFormatKHR> formats;
    UTL::vector<VkPresentModeKHR> presentModes;
};

}

class vulkanSwapChain {
public:
    constexpr static u32 buffer_count{ 3 };
    constexpr static VkFormat default_back_buffer_format{ VK_FORMAT_UNDEFINED };

    vulkanSwapChain() = default;

    explicit vulkanSwapChain(const VkPhysicalDevice& physical_device, const VkDevice& device, const VkSurfaceKHR& surface, const PLATFORM::window& window)
        : _physical_device(physical_device), _device(device), _surface(surface), _window(window) {
        assert(_window.handle());
    }

    DISABLE_COPY_AND_MOVE(vulkanSwapChain);

    ~vulkanSwapChain() { release(); }

    void create(); // query + choose + create
    void recreate(); // resizing ...
    void release();

    [[nodiscard]] const VkSurfaceFormatKHR& surfaceFormat() const { return _surface_format; }
    [[nodiscard]] const VkPresentModeKHR& presentMode() const { return _present_mode; }
    [[nodiscard]] const VkSwapchainKHR& swapchain() const { return _swap_chain; }

private:
    void createImageViews();
    DETAIL::SwapChainSupportDetails querySwapChainSupport() const;
    void chooseSwapSurfaceFormat(const UTL::vector<VkSurfaceFormatKHR>&);
    void chooseSwapPresentMode(const UTL::vector<VkPresentModeKHR>&);
    void chooseSwapExtent(const VkSurfaceCapabilitiesKHR&);

    VkSwapchainKHR                      _swap_chain {VK_NULL_HANDLE};

    VkPhysicalDevice                    _physical_device{ VK_NULL_HANDLE };
    VkDevice                            _device{ VK_NULL_HANDLE };
    VkSurfaceKHR                        _surface{ VK_NULL_HANDLE };
    PLATFORM::window		            _window{};

    VkSurfaceFormatKHR                  _surface_format{};
    VkPresentModeKHR                    _present_mode{};
    VkExtent2D                          _extent{};

    // VkImages of swapchain will be managed by GPU driver,
    UTL::vector<VkImage>                _images{};
    // However, we have to manually create and destroy the image views
    UTL::vector<vulkanImageView>        _image_views{};
};

}
