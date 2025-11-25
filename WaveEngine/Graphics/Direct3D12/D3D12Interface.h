#pragma once
#include "CommonHeaders.h"
#include "Graphics\GraphicsPlatformInterface.h"
#include "D3D12Core.h"

namespace WAVEENGINE::GRAPHICS {

struct platform_interface;

namespace D3D12 {

void get_platform_interface(platform_interface& pi);

}

}
