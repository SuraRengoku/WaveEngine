#pragma once
#include "VulkanCommonHeaders.h"
#include "VulkanQueue.h"
#include "VulkanResources.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

#ifdef _DEBUG
constexpr bool enableValidationLayers = true;
#else
constexpr bool enableValidationLayers = false;
#endif

const UTL::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const UTL::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

/**
 * Instance-level context
 * - Valid from vkCreateInstance to vkDestroyInstance
 * - Used by WSI / Surface / Debug utilities
 */
struct instanceContext {
    VkInstance                          _instance{ VK_NULL_HANDLE };
	VkAllocationCallbacks*				_allocator{};
};

/*
 * Physical device / adapter information
 * - Immutable after selection
 */
struct adapterContext {
    VkPhysicalDevice                    _physicalDevice{ VK_NULL_HANDLE };
    VkPhysicalDeviceProperties2         _properties{};
    VkPhysicalDeviceFeatures2           _features{};
    VkPhysicalDeviceMemoryProperties2   _memoryProperties{};
};

/*
 * Logical device context
 * - Valid from vkCreateDevice to VkDestroyDevice
 * - Used by almost all GPU resource modules
 */
struct deviceContext {
    VkDevice                            _device{ VK_NULL_HANDLE };
    vulkanQueue                         _graphicsQueue{};
    vulkanQueue                         _presentQueue{};
	VkAllocationCallbacks*				_allocator{};
};

/*
    Frame-level Vulkan execution context.

    This structure represents per-frame, CPU–GPU synchronized execution state.
    It is valid only within the lifetime of a single frame-in-flight and must
    not be cached or accessed outside the owning frame loop.

    Responsibilities:
    - Identify the current frame-in-flight via frame_index
    - Provide the command buffer used for recording GPU commands this frame
    - Provide the fence used to synchronize CPU submission and GPU completion

    Notes:
    - frame_index refers to the frame-in-flight index, NOT swapchain image index
    - The command buffer must be in the recording state when handed to render code
    - The fence must be signaled when GPU execution of this frame has completed

    This context does not own any Vulkan resources.
    Resource creation and destruction are managed by higher-level systems.
 */
struct frameContext {
    u32                                 _frameIndex{};
    VkCommandBuffer                     _cmd;
    VkFence                             _fence;
};

/*
    VulkanContext
     ├── VkInstance
     ├── VkPhysicalDevice
     ├── VkDevice
     ├── QueueManager
     │    ├── graphicsQueue
     │    └── presentQueue
     ├── DescriptorManager
     │    ├── immutable_pool
     │    ├── per_scene_pool
     │    ├── per_frame_pool[N]
     │    └── per_draw_pool
     ├── FrameManager
     │    └── VulkanFrameContext[N]
     └── SurfaceManager
          └── vulkanSurface[]
 */
class vulkanContext {
public:
    bool initialize();
    void shutdown();

    // -- instance / device --
    VkInstance instance() const { return _instanceContext._instance; }
    VkPhysicalDevice physical_device() const { return _adapterContext._physicalDevice; }
    VkDevice device() const { return _deviceContext._device; }

    // -- queues --
	vulkanQueue& graphics_queue() { return _deviceContext._graphicsQueue; }
	vulkanQueue& present_queue() { return _deviceContext._presentQueue; }
	const vulkanQueue& graphics_queue() const { return _deviceContext._graphicsQueue; }
	const vulkanQueue& present_queue() const { return _deviceContext._presentQueue; }

    // initialization function
    VkResult createInstance();
    VkResult pickPhysicalDevice();
    VkResult createLogicalDevice();

	// utilities
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
												VkDebugUtilsMessageTypeFlagsEXT messageType,
												const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
												void* pUserData);
	static UTL::vector<const char*> getRequiredExtension();
	static bool checkValidationLayersSupport();
	void setupDebugMessenger() const;
	static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
												const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
												const VkAllocationCallbacks* pAllocator,
												VkDebugUtilsMessengerEXT* pDebugMessenger);
	static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback,
											const VkAllocationCallbacks* pAllocator);

	// getter when vulkanContext object is non-const
	instanceContext& instance_context() { return _instanceContext; }
	adapterContext& adapter_context() { return _adapterContext; }
	deviceContext& device_context() { return _deviceContext; }
	// getter when vulkanContext object is const
	const instanceContext& instance_context() const { return _instanceContext; }
	const adapterContext& adapter_context() const { return _adapterContext; }
	const deviceContext& device_context() const { return _deviceContext; }

private:
    instanceContext                     _instanceContext{ VK_NULL_HANDLE };
    adapterContext						_adapterContext{ VK_NULL_HANDLE };
    deviceContext                       _deviceContext{ VK_NULL_HANDLE };

	VkSampleCountFlagBits				_vkMSAASamples = VK_SAMPLE_COUNT_1_BIT;
	static VkDebugUtilsMessengerEXT		_callback;
};

}
