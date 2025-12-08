#pragma once

#include "D3D12CommonHeaders.h"

#include "D3D12Shaders.h"
#include "D3D12Core.h"

namespace WAVEENGINE::GRAPHICS::D3D12::POSTP {

bool initialize();

void shutdown();

void post_process_render(id3d12GraphicsCommandList* cmd_list, D3D12_CPU_DESCRIPTOR_HANDLE target_rtv);

}
