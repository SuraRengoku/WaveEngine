#include "VulkanContext.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

//////////////////////////////////////////////// UTILITIES //////////////////////////////////////////////////

VkDebugUtilsMessengerEXT vulkanContext::_callback = VK_NULL_HANDLE;

VKAPI_ATTR VkBool32 VKAPI_CALL vulkanContext::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {
#ifdef _DEBUG
	debug_error("::VULKAN:ERROR Validation layer: ");
	debug_error(pCallbackData->pMessage);
	debug_error("\n");
#endif
	return VK_FALSE;
}

UTL::vector<const char*> vulkanContext::getRequiredExtension() {
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

bool vulkanContext::checkValidationLayersSupport() {
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

void vulkanContext::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
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

void vulkanContext::setupDebugMessenger() const{
	if (!enableValidationLayers)
		return;
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	VKCall(CreateDebugUtilsMessengerEXT(_instanceContext._instance, &createInfo, nullptr, &_callback), "::VULKAN:ERROR Failed to set up debug callback\n");

#ifdef _DEBUG
	debug_output("::VULKAN:INFO debug messenger successfully set up\n");
#endif
}

VkResult vulkanContext::CreateDebugUtilsMessengerEXT(VkInstance instance,
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

void vulkanContext::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback,
	const VkAllocationCallbacks* pAllocator) {
	auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
	if (func != nullptr)
		func(instance, callback, pAllocator);
}

/////////////////////////////////////////////// FUNCTIONS ////////////////////////////////////////////////////

bool vulkanContext::initialize() {
	bool result = true;
	VKCall(createInstance(), "::VULKAN:ERROR Failed to create Vulkan Instance\n");
	VKCall(pickPhysicalDevice(), "::VULKAN:ERROR Failed to pick a physical device\n");
	VKCall(createLogicalDevice(), "::VULKAN:ERROR Failed to create a logical device\n");
	return result;
}

void vulkanContext::shutdown() {
	vkDestroyInstance(_instanceContext._instance, _instanceContext._allocator);
	vkDestroyDevice(_deviceContext._device, _deviceContext._allocator);
}

VkResult vulkanContext::createInstance() {
#if _DEBUG
    VKbCall(enableValidationLayers && checkValidationLayersSupport(), "::VULKAN:ERROR Validation layers requested but not available\n");
#endif

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

    VKCall(vkCreateInstance(&instanceInfo, nullptr, &_instanceContext._instance), "::VULKAN:ERROR Failed to create instance\n");

    u32 extensionsCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);
    UTL::vector<VkExtensionProperties> extensions(extensionsCount);

    vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, extensions.data());

#if _DEBUG
    debug_output("Available extensions: \n");
    for (const auto& extension : extensions) {
        debug_output("\t");
        debug_output(extension.extensionName);
        debug_output("\n");
    }
    debug_output("::VULKAN:INFO Instance Created\n");
#endif

    return VK_SUCCESS;
}

VkResult vulkanContext::pickPhysicalDevice() {
    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(_instanceContext._instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
#ifdef _DEBUG
        debug_error("::VULKAN:ERROR Failed to find GPUs with Vulkan support\n");
#endif
        return VK_NOT_READY;
    }
    UTL::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_instanceContext._instance, &deviceCount, devices.data());

    std::multimap<int, VkPhysicalDevice> candidates;
    for (const auto& device : devices) {
        int score = VKX::rateDeviceSuitability(device);
        candidates.insert(std::make_pair(score, device));
    }

    if (candidates.rbegin()->first > 0) {
        _adapterContext._physicalDevice = candidates.rbegin()->second;
        _vkMSAASamples = VKX::getMaxUsableSampleCount(_adapterContext._physicalDevice);
    	_adapterContext._properties = VKX::findPhysicalDeviceProperties(_adapterContext._physicalDevice);
    	_adapterContext._memoryProperties = VKX::findPhysicalDeviceMemoryProperties(_adapterContext._physicalDevice);
    	_adapterContext._features = VKX::findPhysicalDeviceFeatures(_adapterContext._physicalDevice);
#ifdef _DEBUG
        debug_output("::VULKAN:INFO Physical device picked up\n");
#endif
        return VK_SUCCESS;
    }

#ifdef _DEBUG
    debug_error("::VULKAN:ERROR Failed to find a suitable GPU\n");
#endif
    return VK_NOT_READY;
}

VkResult vulkanContext::createLogicalDevice() {
    	VKX::QueueFamilyIndices indices = VKX::findQueueFamilies(_adapterContext._physicalDevice);
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
	// TODO get linux supported extensions
#elif defined(__ANDROID__)
	// TODO get android supported extensions
#endif

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<u32>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	}

	VKCall(vkCreateDevice(_adapterContext._physicalDevice, &createInfo, nullptr, &_deviceContext._device), "::VULKAN:ERROR Failed to create vulkan device\n");

	_deviceContext._graphicsQueue.initialize(_deviceContext._device, indices.graphicsFamily);
	// assume that present queue is same as graphics queue
	_deviceContext._presentQueue.initialize(_deviceContext._device, indices.graphicsFamily);

#ifdef _DEBUG
	debug_output("::VULKAN:INFO Logical device created\n");
#endif

	return VK_SUCCESS;
}

}
