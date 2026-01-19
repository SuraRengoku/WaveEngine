#pragma once

#include "CommonHeaders.h"
#include "Graphics\Renderer.h"
#include "Platform\Window.h"

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#pragma comment(lib, "vulkan-1.lib")
#endif

#include <vulkan/vulkan.h>
#include <iostream>

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

template<typename... Args>
inline void debug_error(const char* format, Args&&... args) {
#if _DEBUG
	char buffer[512];
	snprintf(buffer, sizeof(buffer), format, std::forward<Args>(args)...);
#ifdef _WIN32
	OutputDebugStringA(buffer);
#else
	std::cerr << buffer;
#endif
#endif
}

inline void debug_error(const char* message) {
#if _DEBUG
#ifdef _WIN32
	OutputDebugStringA(message);
#else
	std::cerr << message;
#endif
#endif
}

template<typename ...Args>
inline void debug_output(const char* format, Args&&... args) {
#if _DEBUG
	char buffer[512];
	snprintf(buffer, sizeof(buffer), format, std::forward<Args>(args)...);
#ifdef _WIN32
	OutputDebugStringA(buffer);
#else
	std::cout << buffer;
#endif
#endif
}

inline void debug_output(const char* message) {
#if _DEBUG
#ifdef _WIN32
	OutputDebugStringA(message);
#else
	std::cout << message;
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
#define VKCall(call, message) \
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
#define VK_MOVE_STRUCT(str) str = other.str; other.str = {};
#define VK_DEFINE_PTR_TYPE_OPERATOR(ptr) operator decltype(ptr)() const { return ptr; }
#define VK_DEFINE_ADDRESS_FUNCTION(ptr) const decltype(ptr)* Address() const { return &ptr; }

#define VK_DESTROY_PTR_BY(Func, device, ptr) if(ptr) { Func(device, ptr, nullptr); ptr = VK_NULL_HANDLE; }

////////////////////////////////////////// MOVE CONSTRUCTOR ///////////////////////////////////////

#define VK_MOVE_CTOR1(cName, p0)											\
		cName(cName&& other) noexcept { VK_MOVE_PTR(p0) }

#define VK_MOVE_CTOR2(cName, p0, p1)										\
		cName(cName&& other) noexcept { VK_MOVE_PTR(p0) VK_MOVE_PTR(p1) }

#define VK_MOVE_CTOR3(cName, p0, p1, p2)									\
		cName(cName&& other) noexcept {										\
			VK_MOVE_PTR(p0) VK_MOVE_PTR(p1) VK_MOVE_PTR(p2)					\
		}

#define VK_MOVE_CTOR4(cName, p0, p1, p2, p3)								\
		cName(cName&& other) noexcept {										\
			VK_MOVE_PTR(p0) VK_MOVE_PTR(p1)									\
			VK_MOVE_PTR(p2) VK_MOVE_PTR(p3)									\
		}

#define VK_MOVE_CTOR5(cName, p0, p1, p2, p3, p4)							\
		cName(cName&& other) noexcept {										\
			VK_MOVE_PTR(p0) VK_MOVE_PTR(p1) VK_MOVE_PTR(p2)					\
			VK_MOVE_PTR(p3) VK_MOVE_PTR(p4)									\
		}

// mixed
#define VK_MOVE_CTOR_CUSTOM(cName, ...)										\
		cName(cName&& other) noexcept { __VA_ARGS__ }

//////////////////////////////////////////// MOVE ASSIGNMENT ///////////////////////////////////////

#define VK_MOVE_ASSIGN1(cName, p0)											\
		cName& operator=(cName&& other) noexcept {							\
			if(this != &other) {											\
				VK_MOVE_PTR(p0)												\
			}																\
			return *this;													\
		}

#define VK_MOVE_ASSIGN2(cName, p0, p1)										\
		cName& operator=(cName&& other) noexcept {							\
			if(this != &other) {											\
				VK_MOVE_PTR(p0) VK_MOVE_PTR(p1)								\
			}																\
			return *this;													\
		}

#define VK_MOVE_ASSIGN3(cName, p0, p1, p2)									\
		cName& operator=(cName&& other) noexcept {							\
			if(this != &other) {											\
				VK_MOVE_PTR(p0) VK_MOVE_PTR(p1)								\
				VK_MOVE_PTR(p2)												\
			}																\
			return *this;													\
		}

#define VK_MOVE_ASSIGN4(cName, p0, p1, p2, p3)								\
		cName& operator=(cName&& other) noexcept {							\
			if(this != &other) {											\
				VK_MOVE_PTR(p0) VK_MOVE_PTR(p1)								\
				VK_MOVE_PTR(p2) VK_MOVE_PTR(p3)								\
			}																\
			return *this;													\
		}

#define VK_MOVE_ASSIGN5(cName, p0, p1, p2, p3, p4)							\
		cName& operator=(cName&& other) noexcept {							\
			if(this != &other) {											\
				VK_MOVE_PTR(p0) VK_MOVE_PTR(p1)								\
				VK_MOVE_PTR(p2) VK_MOVE_PTR(p3)								\
				VK_MOVE_PTR(p4)												\
			}																\
			return *this;													\
		}

// mixed
#define VK_MOVE_ASSIGN_CUSTOM(cName, ...)										\
		cName& operator=(cName&& other) noexcept { __VA_ARGS__ }

#include "VulkanHelpers.h"
