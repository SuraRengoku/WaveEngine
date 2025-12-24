#include "VulkanSurface.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

bool vulkanSurface::create(const instanceContext& ctx) {
#ifdef _WIN32
    HWND hwnd{ static_cast<HWND>(_window.handle()) };

    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = hwnd;
    createInfo.hinstance = GetModuleHandle(nullptr);
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    VKCall(vkCreateWin32SurfaceKHR(ctx._instance, &createInfo, ctx._allocator, &_surface), "::VULKAN: failed to create a Win32 surface");
#elif defined(__APPLE__)
    VkMetalSurfaceCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    // TODO
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    VKCall(vkCreateMetalSurfaceEXT(instance, &createInfo, nullptr, &_surface), "::VULKAN: failed to create a Metal surface");
#elif defined(__linux__)

    // TODO

#elif defined(__ANDROID__)
    VkAndroidSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    // TODO
    createInfo.sNext = nullptr;
    createInfo.flags = 0;
    VKCall(vkCreateAndroidSurfaceKHR(instance, &createInfo, nullptr, &_surface), "::VULKAN: failed to create an Android surface");
#else
    debug_output("Unsupported platform");
#endif

    return true;
}

void vulkanSurface::destroy(const instanceContext& ctx) {
    if (_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(ctx._instance, _surface, ctx._allocator);
        _surface = VK_NULL_HANDLE;
    }
}
}
