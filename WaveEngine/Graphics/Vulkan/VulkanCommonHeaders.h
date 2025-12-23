#pragma once

#include "CommonHeaders.h"
#include "Graphics\Renderer.h"
#include "Platform\Window.h"
#include <vulkan/vulkan.h>
#include <iostream>

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#pragma comment(lib, "vulkan-1.lib")
#endif

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#include <stb_image.h>

// VMA(Vulkan Memory Allocator) Configuration
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

namespace WAVEENGINE::GRAPHICS::VULKAN {

constexpr u32 frame_buffer_count{ 3 };

}

#ifdef _WIN32
#define DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
#if defined(__i386__) || defined(__x86_64__)
#define DEBUG_BREAK() __asm__ __volatile__("int3")
#else
#include <signal.h>
#define DEBUG_BREAK() raise(SIGTRAP)
#endif
#else
#include <signal.h>
#define DEBUG_BREAK() raise(SIGTRAP)
#endif

inline void debug_output(const char* message) {
#if _DEBUG
#ifdef _WIN32
	OutputDebugStringA(message);
#else
	std::cerr << message;
#endif
#endif
}

#ifdef _DEBUG
#define VMA_DEBUG_MARGIN 16
#define VMA_DEBUG_DETECT_CORRUPTION 1
#define VMA_DEBUG_LOG(format, ...)								\
	do {														\
		char buffer[256];										\
		snprintf(buffer, sizeof(buffer), format, __VA_ARGS__);	\
		debug_output(buffer);									\
	} while(0)

#endif

#ifdef _DEBUG
#ifndef VKbCall
#define VKbCall(call, message)											\
	do {																\
		if(!(call)) {													\
			char line_number[32];										\
			snprintf(line_number, sizeof(line_number), "%u", __LINE__); \
			debug_output("Error in: ");									\
			debug_output(__FILE__);										\
			debug_output("\nLine: ");									\
			debug_output(line_number);									\
			debug_output("\n");											\
			debug_output(TOSTRING(call));								\
			debug_output("\n");											\
			debug_output(message);										\
			debug_output("\n");											\
			DEBUG_BREAK();												\
		}																\
	} while(0)
#endif
#else
#ifndef VKbCall
#define VKbCall(call, message) \
	do { (void)(call); (void)(message); } while(0) 
#endif
#endif

#ifdef _DEBUG
#ifndef VKCall
#define VKCall(call, message)											\
	do {																\
		VkResult result = call;											\
		if(result != VK_SUCCESS) {										\
			char line_number[32];										\
			snprintf(line_number, sizeof(line_number), "%u", __LINE__); \
			debug_output("Error in: ");									\
			debug_output(__FILE__);										\
			debug_output("\nLine: ");									\
			debug_output(line_number);									\
			debug_output("\n");											\
			debug_output(TOSTRING(call));								\
			debug_output("\n");											\
			debug_output(message);										\
			debug_output("\n");											\
			DEBUG_BREAK();												\
		}																\
	} while(0)
#endif
#else
#ifndef VKCall
#define VKCall(call, message) call \
	do { (void)(call); (void)(message); } while(0)
#endif
#endif

#ifdef _DEBUG
// set the name of the vk object and output a debug information
#define NAME_VK_OBJECT(device, obj, type, name)											\
	do {																				\
		VkDebugUtilsObjectNameInfoEXT nameInfo = {};									\
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;			\
		nameInfo.objectType = type;														\
		nameInfo.objectHandle = (u64)obj;												\
		nameInfo.pObjectName = name;													\
		if(vkSetDebugUtilsObjectNameEXT) {												\
			vkSetDebugUtilsObjectNameEXT(device, &nameInfo);							\
		}																				\
	} while (0)

// the indexed variant will include the index in the name of the object
#define NAME_VK_OBJECT_INDEXED(device, obj, idx, type, name)							\
	do {																				\
		char buffer[256];																\
		snprintf(buffer, sizeof(buffer), "%s [%u]", name, idx);							\
		NAME_VK_OBJECT(device, obj, type, buffer);										\
	} while(0)

#else 

#define NAME_VK_OBJECT(device, obj, type, name)
#define NAME_VK_OBJECT_INDEXED(device, obj, idx, type, name)

#endif

#define VK_MOVE_PTR(ptr) ptr = other.ptr; other.ptr = VK_NULL_HANDLE;
#define VK_MOVE_VALUE(val) val = other.val; other.val = 0;
#define VK_DEFINE_PTR_TYPE_OPERATOR(ptr) operator decltype(ptr)() const { return ptr; }
#define VK_DEFINE_ADDRESS_FUNCTION(ptr) const decltype(ptr)* Address() const { return &ptr; }

#define VK_DESTROY_PTR_BY(Func, device, ptr) if(ptr) { Func(device, ptr, nullptr); ptr = VK_NULL_HANDLE; }

#include "VulkanHelpers.h"
