#pragma once
#include "VulkanCommonHeaders.h"
#include "VulkanQueue.h"
#include "VulkanCommand.h"
#include "VulkanSync.h"

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
	// Ray Tracing extension will be added on initialization (if available)
};

#ifdef VK_KHR_ray_tracing_pipeline
constexpr const char* VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME_STR = VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME;
constexpr const char* VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME_STR = VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME;
constexpr const char* VK_KHR_RAY_QUERY_EXTENSION_NAME_STR = VK_KHR_RAY_QUERY_EXTENSION_NAME;
constexpr const char* VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME_STR = VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME;
#endif

/*
 * Instance-level context
 *
 * Lifetime: Valid from vkCreateInstance to vkDestroyInstance
 *
 * Responsibilities:
 * - Manages the global Vulkan instance handle
 * - Provides allocation callbacks for instance-level resources
 *
 * Usage:
 * - Window Surface Integration (WSI)
 * - Physical device enumeration
 * - Debug utilities and validation layers
 * - Extension querying
 *
 * Notes:
 * - This context is shared across all surfaces and devices
 * - Should be created first and destroyed last in the Vulkan lifecycle
 * - Thread-safe as long as the instance handle is not modified concurrently
 */
struct instanceContext {
    VkInstance                          _instance{ VK_NULL_HANDLE };
	VkAllocationCallbacks*				_allocator{};
};

/*
 * Physical device / adapter information
 *
 * Lifetime: Immutable after selection via vkEnumeratePhysicalDevices
 *
 * Responsibilities:
 * - Stores the selected physical device (GPU) handle
 * - Caches device properties, features, and memory characteristics
 *
 * Usage:
 * - Query GPU capabilities before logical device creation
 * - Determine supported formats, sample counts, and limits
 * - Memory type selection for buffer/image allocation
 * - Feature validation (e.g., anisotropic filtering, geometry shaders)
 *
 * Notes:
 * - Read-only after initialization (no modification allowed)
 * - Properties are queried once and cached for performance
 * - Supports both Vulkan 1.0 and 1.1+ property query methods
 */
struct adapterContext {
    VkPhysicalDevice                    _physicalDevice{ VK_NULL_HANDLE };
#if VK_VERSION_1_1
    VkPhysicalDeviceProperties2         _properties{};
    VkPhysicalDeviceFeatures2           _features{};
    VkPhysicalDeviceMemoryProperties2   _memoryProperties{};
#else
	VkPhysicalDeviceProperties          _properties{};
	VkPhysicalDeviceFeatures            _features{};
	VkPhysicalDeviceMemoryProperties    _memoryProperties{};
#endif
};

/*
 * Logical device context
 *
 * Lifetime: Valid from vkCreateDevice to vkDestroyDevice
 *
 * Responsibilities:
 * - Manages the logical device handle (primary interface to GPU)
 * - Provides graphics and present queue handles
 * - Supplies allocation callbacks for device-level resources
 *
 * Usage:
 * - Creation of all GPU resources (buffers, images, pipelines, etc.)
 * - Command buffer submission via queue handles
 * - Synchronization primitive management
 * - Memory allocation operations
 *
 * Notes:
 * - The logical device is created from a selected physical device
 * - Graphics and present queues may be the same or different families
 * - All resources created from this device must be destroyed before device shutdown
 * - Queue operations are internally synchronized via mutex in vulkanQueue
 */
struct deviceContext {
    VkDevice                            _device{ VK_NULL_HANDLE };
    vulkanQueue                         _graphicsQueue{};
    vulkanQueue                         _presentQueue{};
	VkAllocationCallbacks*				_allocator{};
};

/*
 * Frame-level Vulkan execution context
 *
 * Lifetime: Valid within a single frame-in-flight (persistent across multiple render passes)
 *
 * Responsibilities:
 * - Track the current frame-in-flight index (NOT swapchain image index)
 * - Provide command buffer for recording GPU commands
 * - Manage CPU-GPU synchronization via fence and semaphores
 * - Cache render encoders per swapchain image to avoid reconstruction overhead
 *
 * Usage:
 * - Used in render loop to record and submit frame commands
 * - Frame index cycles through [0, frame_buffer_count) across frames
 * - Command buffer must be reset and begun before use each frame
 * - Fence ensures GPU completes work before CPU reuses resources
 * - Semaphores coordinate image acquisition and presentation
 *
 * Synchronization Flow:
 * 1. Wait on fence (CPU waits for GPU to finish previous frame)
 * 2. Acquire swapchain image (signals image_available_semaphore)
 * 3. Record and submit commands (waits on image_available, signals render_finished)
 * 4. Present image (waits on render_finished_semaphore)
 *
 * Render Encoder Caching:
 * - render_encoders[image_index] caches encoder for each swapchain image
 * - Avoids redundant construction when framebuffer/renderPass don't change
 * - Typical swapchain has 2-3 images, so array size is frame_buffer_count
 * - First access initializes encoder, subsequent accesses reuse it
 *
 * Notes:
 * - frame_index refers to frame-in-flight (0 to frame_buffer_count-1)
 * - Swapchain image index is obtained via vkAcquireNextImageKHR (may differ from frame_index)
 * - This context does NOT own Vulkan resources (managed by higher-level systems)
 * - Command buffer must be in recording state when handed to render code
 * - Fence must be signaled when GPU execution completes
 * - Encoder cache optimizes common case where renderPass/framebuffer remain stable
 *
 * Memory Layout Considerations:
 * - Total size: ~60 bytes (base) + frame_buffer_count * sizeof(optional<encoder>)
 * - With 3 cached encoders: ~160 bytes per frame context
 * - Fits comfortably in 3 cache lines (64 bytes each)
 * - Sequential access pattern ensures good cache performance
 */
using swapchainEncoder = std::array<std::optional<vulkanRenderEncoder>, frame_buffer_count>;

struct frameContext {
	u32								    frame_index{ u32_invalid_id };
    vulkanCommandBuffer                 graphics_cmd_buffer;

	vulkanFence						    fence;
	vulkanSemaphore					    image_available_semaphore;
	vulkanSemaphore					    render_finished_semaphore;

	vulkanRenderEncoder					render_encoder;
	// Cache render encoders per swapchain image to avoid per-frame reconstruction
	// Note: vulkanRenderEncoder internally stores VkCommandBuffer as a handle reference
    //swapchainEncoder                    render_encoders;
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
	vulkanContext() = default;

    bool initialize();
    void shutdown() const;

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

	// ================================== Ray Tracing Feature ===================================

	static PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
	static PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
	static PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;

	bool loadRayTracingFunctions();

	bool isRayTracingSupported() const { return _rayTracingSupported; }

	// ==========================================================================================

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

	bool								_rayTracingSupported{ false };
};

}
