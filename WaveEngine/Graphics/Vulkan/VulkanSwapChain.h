#pragma once
#include "VulkanCommonHeaders.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

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
    constexpr static VkFormat default_back_buffer_format{};

    vulkanSwapChain() = default;

    explicit vulkanSwapChain(const VkPhysicalDevice& physical_device, const VkSurfaceKHR& surface, const PLATFORM::window& window)
        : _physical_device(physical_device), _surface(surface), _window(window) {
        assert(_window.handle());
        querySwapChainSupport();
        chooseSwapSurfaceFormat();
        chooseSwapPresentMode();
    }

    ~vulkanSwapChain() { release(); }

    void querySwapChainSupport();
    void chooseSwapSurfaceFormat();
    void chooseSwapPresentMode();
    void chooseSwapExtent();

    [[nodiscard]] VkSurfaceFormatKHR surfaceFormat() const { return _surface_format; }
    [[nodiscard]] VkPresentModeKHR presentMode() const { return _present_mode; }
    [[nodiscard]] VkSwapchainKHR swapchain() const { return _swap_chain; }

private:
    void create();
    void release();

    VkSwapchainKHR                      _swap_chain {VK_NULL_HANDLE};
    VkPhysicalDevice                    _physical_device{ VK_NULL_HANDLE };
    VkDevice                            _device{ VK_NULL_HANDLE };
    VkSurfaceKHR                        _surface{ VK_NULL_HANDLE };
    PLATFORM::window		            _window{};
    DETAIL::SwapChainSupportDetails     _chain_support_details{};
    VkSurfaceFormatKHR                  _surface_format{};
    VkPresentModeKHR                    _present_mode{};
    VkExtent2D                          _extent{};

    UTL::vector<VkImage>                _images{};
    UTL::vector<VkImageView>            _image_views{};
};

}
