#pragma once
#include "CommonHeaders.h"
#include "Graphics\GraphicsPlatformInterface.h"
#include "VulkanCore.h"

namespace WAVEENGINE::GRAPHICS{

struct platform_interface;

namespace VULKAN {

void get_platform_interface(platform_interface& pi);

}

}
