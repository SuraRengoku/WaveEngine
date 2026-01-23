#include "VulkanContext.h"

namespace WAVEENGINE::GRAPHICS::VULKAN {

//////////////////////////////////////////////// UTILITIES //////////////////////////////////////////////////

VkDebugUtilsMessengerEXT vulkanContext::_callback = VK_NULL_HANDLE;

VkBool32 VKAPI_CALL vulkanContext::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
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

	loadRayTracingFunctions();

	return result;
}

void vulkanContext::shutdown() const {
	vkDeviceWaitIdle(_deviceContext._device);
	vkDestroyDevice(_deviceContext._device, _deviceContext._allocator);
#ifdef _DEBUG
	if (enableValidationLayers && _callback != VK_NULL_HANDLE) {
		DestroyDebugUtilsMessengerEXT(_instanceContext._instance, _callback, _instanceContext._allocator);
	}
#endif
	vkDestroyInstance(_instanceContext._instance, _instanceContext._allocator);
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

	// TODO add instance callback

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

	// ================================= Ray Tracing Extension ==================================

	// enable Ray Tracing extension
	UTL::vector<const char*> enabledExtensions = deviceExtensions;

	// query supported extensions
	u32 extensionsCount = 0;
	vkEnumerateDeviceExtensionProperties(_adapterContext._physicalDevice, nullptr, &extensionsCount, nullptr);
	UTL::vector<VkExtensionProperties> availableExtensions(extensionsCount);
	vkEnumerateDeviceExtensionProperties(_adapterContext._physicalDevice, nullptr, &extensionsCount, availableExtensions.data());

	bool rtPipelineSupported = false;
	bool rtAccelStructSupported = false;
	bool rtDeferredOpsSupported = false;

	for (const auto& ext : availableExtensions) {
		if (strcmp(ext.extensionName, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) == 0) {
			rtPipelineSupported = true;
		} else if (strcmp(ext.extensionName, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) == 0) {
			rtAccelStructSupported = true;
		} else if (strcmp(ext.extensionName, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME) == 0) {
			rtDeferredOpsSupported = true;
		}
	}

	if (rtPipelineSupported && rtAccelStructSupported && rtDeferredOpsSupported) {
		enabledExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
		enabledExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
		enabledExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);

		enabledExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
		//enabledExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
		//enabledExtensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);

#ifdef _DEBUG
		debug_output("::VULKAN:INFO Ray Tracing extensions will be enabled\n");
#endif
	}

	// =======================================================================================
	
	// =========================== Enable Ray Tracing Feature ===============================

	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures{};
	rtPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
	rtPipelineFeatures.rayTracingPipeline = VK_TRUE;

	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelStructFeatures{};
	accelStructFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	accelStructFeatures.accelerationStructure = VK_TRUE;
	accelStructFeatures.pNext = &rtPipelineFeatures;

	VkPhysicalDeviceBufferDeviceAddressFeaturesEXT bufferDeviceAddressFeatures{};
	bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR;
	bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;

	if (rtPipelineSupported && rtAccelStructSupported && rtDeferredOpsSupported) {
		bufferDeviceAddressFeatures.pNext = &accelStructFeatures;
	}

	VkPhysicalDeviceFeatures2 deviceFeatures2{};
	deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures2.features = deviceFeatures;

	if (rtPipelineSupported && rtAccelStructSupported && rtDeferredOpsSupported) {
		deviceFeatures2.pNext = &bufferDeviceAddressFeatures;
	}

	// =======================================================================================

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = nullptr;
	createInfo.pNext = &deviceFeatures2;
#ifdef _WIN32
	createInfo.enabledExtensionCount = static_cast<u32>(enabledExtensions.size());
	createInfo.ppEnabledExtensionNames = enabledExtensions.data();
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

/////////////////////////////////////////// RAY TRACING FEATURES /////////////////////////////////////////////////

PFN_vkCreateRayTracingPipelinesKHR vulkanContext::vkCreateRayTracingPipelinesKHR = nullptr;
PFN_vkGetRayTracingShaderGroupHandlesKHR vulkanContext::vkGetRayTracingShaderGroupHandlesKHR = nullptr;
PFN_vkCmdTraceRaysKHR vulkanContext::vkCmdTraceRaysKHR = nullptr;

bool vulkanContext::loadRayTracingFunctions() {
	vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(
		vkGetDeviceProcAddr(_deviceContext._device, "vkCreateRayTracingPipelinesKHR"));

	vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(
		vkGetDeviceProcAddr(_deviceContext._device, "vkGetRayTracingShaderGroupHandlesKHR"));

	vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(
		vkGetDeviceProcAddr(_deviceContext._device, "vkCmdTraceRaysKHR"));

	_rayTracingSupported = (vkCreateRayTracingPipelinesKHR != nullptr);

#ifdef _DEBUG
	if (_rayTracingSupported) {
		debug_output("::VULKAN::INFO Ray tracing functions loaded successfully\n");
		debug_output("  - vkCreateRayTracingPipelinesKHR: %p\n", (void*)vkCreateRayTracingPipelinesKHR);
		debug_output("  - vkGetRayTracingShaderGroupHandlesKHR: %p\n", (void*)vkGetRayTracingShaderGroupHandlesKHR);
		debug_output("  - vkCmdTraceRaysKHR: %p\n", (void*)vkCmdTraceRaysKHR);
	} else {
		debug_output("::VULKAN:WARNING Ray Tracing not supported on this device\n");
		debug_output("  - Device extensions may not be enabled\n");
		debug_output("  - Check if VK_KHR_ray_tracing_pipeline was enabled during device creation\n");
	}
#endif

	return _rayTracingSupported;
}

}
