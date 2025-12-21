#include "VulkanCore.h"
#include "VulkanSurface.h"

namespace WAVEENGINE::GRAPHICS::VULKAN::CORE {

namespace {

#ifdef _DEBUG
constexpr bool enableValidationLayers = true;
#else
constexpr bool enableValidationLayers = false;
#endif

using surfaceCollection = UTL::freeList<vulkanSurface>;

VkInstance							vk_instance;
VkPhysicalDevice					vk_physicalDevice;
VkDevice							vk_device;
VkDebugUtilsMessengerEXT			callback;
surfaceCollection					surfaces;

vulkanQueue							graphicsQueue;
vulkanQueue							presentQueue;

// Texture
VkSampleCountFlagBits				vk_msaa_samples = VK_SAMPLE_COUNT_1_BIT;

const UTL::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const UTL::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

descriptorPool						immutable_pool{};								// never update
descriptorPool						per_scene_pool{};								// infrequently update
descriptorPool						per_frame_pool[frame_buffer_count]{};			// frequently update
descriptorPool						per_draw_pool{};								// most frequently update

u32									deferred_releases_flag[frame_buffer_count];
std::mutex							deferred_releases_mutex{};

bool check_vulkan_runtime() {
	HMODULE vulkan_dll = LoadLibraryA("vulkan-1.dll");
	if (!vulkan_dll) {
		MessageBoxA(nullptr,
			"Vulkan Runtime not found!\n\n"
			"Please update your GPU drivers or install Vulkan Runtime from:\n"
			"https://vulkan.lunarg.com/sdk/home#windows\n\n"
			"The application will now fall back to Direct3D 12.",
			"Vulkan Required",
			MB_OK | MB_ICONWARNING);
		return false;
	}
	FreeLibrary(vulkan_dll);
	return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
	void* pUserData) {

#ifdef _DEBUG
	debug_output("::VULKAN: validation layer: ");
	debug_output(pCallbackData->pMessage);
	debug_output("\n");
#endif

	return VK_FALSE;
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, 
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback,
	const VkAllocationCallbacks* pAllocator) {
	auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
	if (func != nullptr)
		func(instance, callback, pAllocator);
}

void setupDebugMessenger() {
	if (!enableValidationLayers)
		return;
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	VKCall(CreateDebugUtilsMessengerEXT(vk_instance, &createInfo, nullptr, &callback), "::VULKAN: failed to set up debug callback\n");

#ifdef _DEBUG
	debug_output("::VULKAN: debug messenger successfully set up\n");
#endif
}

bool checkValidationLayersSupport() {
	u32 layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	UTL::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool layerFound = false;
		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}
		if (!layerFound)
			return false;
	}
	return true;
}

UTL::vector<const char*> getRequiredExtension() {
	UTL::vector<const char*> extensions;

	// base surface extension for all platform
	extensions.emplace_back("VK_KHR_surface");

#ifdef _WIN32
	extensions.emplace_back("VK_KHR_win32_surface");
#elif defined(__APPLE__)
	extensions.emplace_back("VK_EXT_metal_surface");
	extensions.emplace_back("VK_KHR_portability_enumeration");
	extensions.emplace_back("VK_KHR_get_physical_device_properties2");
#elif defined(__linux__)
	// Xlib
	extensions.emplace_back("VK_KHR_xlib_surface");
	// Wayland
	// extensions.emplace_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
	// XCB
	// extensions.emplace_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif

#ifdef _DEBUG
	extensions.emplace_back("VK_EXT_debug_utils");
#endif

	return extensions;
}

VkResult createInstance() {

	VKbCall(enableValidationLayers && checkValidationLayersSupport(), "::VULKAN: Validation layers requested but not available\n");

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr; // leave it
	appInfo.pApplicationName = "Integrated";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "WAVE";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	auto requiredExtensions = getRequiredExtension();

	VkInstanceCreateInfo instanceInfo{};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &appInfo;
#ifdef __APPLE__
	instanceInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#else
	instanceInfo.flags = 0;
#endif
	instanceInfo.enabledExtensionCount = static_cast<u32>(requiredExtensions.size());
	instanceInfo.ppEnabledExtensionNames = requiredExtensions.data();

	if (enableValidationLayers) {
		instanceInfo.enabledLayerCount = static_cast<u32>(validationLayers.size());
		instanceInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		instanceInfo.enabledLayerCount = 0;
	}

	VKCall(vkCreateInstance(&instanceInfo, nullptr, &vk_instance), "::VULKAN: failed to create instance\n");

	u32 extensionsCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);
	UTL::vector<VkExtensionProperties> extensions(extensionsCount);

	vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, extensions.data());

#if _DEBUG
	debug_output("available extensions: \n");
	for (const auto& extension : extensions) {
		debug_output("\t");
		debug_output(extension.extensionName);
		debug_output("\n");
	}
	debug_output("::VULKAN: Instance Created\n");
#endif

	return VK_SUCCESS;
}

VkResult pickPhysicalDevice() {
	u32 deviceCount = 0;
	vkEnumeratePhysicalDevices(vk_instance, &deviceCount, nullptr);
	if (deviceCount == 0) {
#ifdef _DEBUG
		debug_output("::VULKAN: failed to find GPUs with Vulkan support\n");
#endif
		return VK_NOT_READY;
	}
	UTL::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(vk_instance, &deviceCount, devices.data());

	std::multimap<int, VkPhysicalDevice> candidates;
	for (const auto& device : devices) {
		int score = VKX::rateDeviceSuitability(device);
		candidates.insert(std::make_pair(score, device));
	}

	if (candidates.rbegin()->first > 0) {
		vk_physicalDevice = candidates.rbegin()->second;
		vk_msaa_samples = VKX::getMaxUsableSampleCount(vk_physicalDevice);
#ifdef _DEBUG
		debug_output("::VULKAN: Physical device picked up\n");
#endif
		return VK_SUCCESS;
	} 

#ifdef _DEBUG
	debug_output("::VULKAN: failed to find a suitable GPU\n");
#endif
	return VK_NOT_READY;
}

// TODO
VkResult createLogicalDevice() {
	VKX::QueueFamilyIndices indices = VKX::findQueueFamilies(vk_physicalDevice);
	UTL::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
	std::set<int> uniqueQueueFamilies = {indices.graphicsFamily};

	// for each queue family, we barely need more than one queue
	// Vulkan assign each queue a priority index to control the sequence of executing
	float queuePriority = 1.0f;
	for (int queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.emplace_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.sampleRateShading = VK_TRUE;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
#ifdef _WIN32
	createInfo.enabledExtensionCount = static_cast<u32>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
#elif defined(__APPLE__) && defined(__arm64__)
	UTL::vector<const char*> requiredExtensions = getRequiredExtension(vk_physicalDevice);
	requiredExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	createInfo.enabledExtensionCount = static_cast<u32>(requiredExtensions);
	createInfo.ppEnabledExtensionNames = requiredExtensions.data();
#elif defined(__linux__)
	// TODO
#elif defined(__ANDROID__)
	// TODO
#endif

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<u32>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	}

	VKCall(vkCreateDevice(vk_physicalDevice, &createInfo, nullptr, &vk_device), "::VULKAN: failed to create vulkan device\n");

	graphicsQueue.initialize(vk_device, indices.graphicsFamily);
	// assume that present queue is same as graphics queue
	presentQueue.initialize(vk_device, indices.graphicsFamily);

#ifdef _DEBUG
	debug_output("::VULKAN: Logical device created\n");
#endif

	return VK_SUCCESS;
}

}

namespace DETAIL {

void deferred_release() {
	const u32 frame_idx{ current_frame_index() };
}

}

bool initialize() {
	// check Vulkan Runtime
	if (!check_vulkan_runtime()) {
		return false;
	}

	VKCall(createInstance(), "::VULKAN: failed to create vulkan instance\n");
#ifdef _DEBUG
	setupDebugMessenger();
#endif
	VKCall(pickPhysicalDevice(), "::VULKAN: failed to pick a physical device\n");
	VKCall(createLogicalDevice(), "::VULKAN: failed to create a logical device\n");

	VKbCall(immutable_pool.initialize(2048, VKX::immutablePoolSizes), "::VULKAN: failed to initialize immutable descriptor pool\n");
	VKbCall(per_scene_pool.initialize(256, VKX::perScenePoolSizes), "::VULKAN: failed to initialize per scene descriptor pool\n");
	for (u32 i{0}; i < frame_buffer_count; ++i) {
		VKbCall(per_frame_pool[i].initialize(512, VKX::perFramePoolSizes), "::VULKAN: failed to initialize per frame descriptor pool\n");
	}
	VKbCall(per_draw_pool.initialize(1024, VKX::perDrawPoolSizes), "::VULKAN: failed to initialize per draw descriptor pool\n");

	return true;
}

void shutdown() {
	
}

VkDevice device() {
	return vk_device;
}

VkQueue graphics_queue() {
	return graphicsQueue.queue();
}

VkQueue present_queue() {
	return presentQueue.queue();
}

u32 current_frame_index() {
	// TODO
	return u32_invalid_id;
}

void set_deferred_releases_flag() {
	
}

surface create_surface(PLATFORM::window window) {
	surface_id id{ surfaces.add(window) };
	surfaces[id].create_surface(vk_instance);

	// validate Present Support
	VKX::QueueFamilyIndices indices = VKX::findQueueFamilies(vk_physicalDevice, surfaces[id].surface());
#if _DEBUG
	if (!indices.hasPresentSupport()) {
		debug_output("::VULKAN: Graphics queue doesn't support present on this surface\n");
		assert(indices.hasPresentSupport());
	}
#endif

	// if we need separate present queue, reinitialize.
	if (indices.presentFamily != graphicsQueue.familyIndex()) {
		presentQueue.initialize(vk_device, indices.presentFamily);
#if _DEBUG
		debug_output("::VULKAN: Using separate present queue\n");
#endif
	}
	return surface{id};
}

void remove_surface(surface_id id) {
	
}

void resize_surface(surface_id id, u32 width, u32 height) {
	
}

u32 surface_width(surface_id id) {
	return 0;
}

u32 surface_height(surface_id id) {
	return 0;
}

void render_surface(surface_id id) {
	
}
}
