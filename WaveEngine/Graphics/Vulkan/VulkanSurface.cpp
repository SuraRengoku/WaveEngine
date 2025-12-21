#include "VulkanSurface.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

bool vulkanSurface::create_surface(VkInstance instance) {
#ifdef _WIN32
    HWND hwnd{ static_cast<HWND>(_window.handle()) };

    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = hwnd;
    createInfo.hinstance = GetModuleHandle(nullptr);
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    VKCall(vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &_surface), "::VULKAN: failed to create a Win32 surface");
#elif defined(__APPLE__)
    VkMetalSurfaceCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    // TODO
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    VKCall(vkCreateMetalSurfaceEXT(instance, &createInfo, nullptr, &_surface), "::VULKAN: failed to create a Metal surface");
#elif defined(__linux__)

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

void vulkanSurface::present() const {

}

void vulkanSurface::resize() {

}

void vulkanSurface::finalize() {

}

void vulkanSurface::release() {

}
}
