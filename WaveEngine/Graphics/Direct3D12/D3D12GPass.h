#pragma once

#include "D3D12CommonHeaders.h"
#include "D3D12Resources.h"
#include "D3D12Helpers.h"

namespace WAVEENGINE::GRAPHICS::D3D12 {
struct d3d12FrameInfo;
class d3d12RenderTexture;
class d3d12DepthStencilBuffer;
}

namespace WAVEENGINE::GRAPHICS::D3D12::GPASS {

bool initialize();
void shutdown();

[[nodiscard]] const d3d12RenderTexture& main_buffer();
[[nodiscard]] const d3d12DepthStencilBuffer& depth_buffer();

void set_size(MATH::u32v2 size);

void depth_prepass(id3d12GraphicsCommandList* cmd_list, const d3d12FrameInfo& info);

void render(id3d12GraphicsCommandList* cmd_list, const d3d12FrameInfo& info);

void add_transitions_for_depth_prepass(D3DX::d3d12ResourceBarrier& barriers);
void add_transitions_for_gpass(D3DX::d3d12ResourceBarrier& barriers);
void add_transitions_for_post_process(D3DX::d3d12ResourceBarrier& barriers);

void set_render_targets_for_depth_prepass(id3d12GraphicsCommandList* cmd_list);

void set_render_targets_for_gpass(id3d12GraphicsCommandList* cmd_list);

}