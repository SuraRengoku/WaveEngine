#include "D3D12GPass.h"

namespace WAVEENGINE::GRAPHICS::D3D12::GPASS {

namespace {

constexpr DXGI_FORMAT		main_buffer_format{ DXGI_FORMAT_R16G16B16A16_FLOAT };
constexpr DXGI_FORMAT		depth_buffer_format{ DXGI_FORMAT_D32_FLOAT };
constexpr MATH::u32v2		initial_dimensions{ 100, 100 };

d3d12RenderTexture			gpass_main_buffer{}; // for colors
d3d12DepthStencilBuffer		gpass_depth_buffer{};
MATH::u32v2					dimensions{ initial_dimensions };

ID3D12RootSignature*		gpass_root_sig{ nullptr };
ID3D12PipelineState*		gpass_pso{ nullptr };

#if _DEBUG
constexpr f32				clear_value[4]{ 0.5f, 0.5f, 0.5f, 1.0f };
#else
constexpr f32				clear_value[4]{ }; // black
#endif

bool create_buffers(MATH::u32v2 size) {
	assert(size.x && size.y);
	
	// release the old one
	gpass_main_buffer.release();
	gpass_depth_buffer.release(); 
	
	D3D12_RESOURCE_DESC desc{};
	desc.Alignment = 0; // 0 is the same as 64KB (or 4MB for MSAA)
	desc.DepthOrArraySize = 1;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	desc.Format = main_buffer_format;
	desc.Width = size.x;
	desc.Height = size.y;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.MipLevels = 0; // make space for all mip levels
	desc.SampleDesc = { 1, 0 };

	// create the color buffer
	{
		d3d12TextureInitInfo info{};
		info.desc = &desc;
		info.initial_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		info.clear_value.Format = desc.Format;
		memcpy(&info.clear_value.Color, &clear_value[0], sizeof(clear_value));

		gpass_main_buffer = d3d12RenderTexture{ info };
	}

	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	desc.Format = depth_buffer_format;
	desc.MipLevels = 1;

	// create the depth buffer 
	{
		d3d12TextureInitInfo info{};
		info.desc = &desc;
		info.initial_state = D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		info.clear_value.Format = desc.Format;
		info.clear_value.DepthStencil.Depth = 0.0f;
		info.clear_value.DepthStencil.Stencil = 0;

		gpass_depth_buffer = d3d12DepthStencilBuffer{ info };
	}

	NAME_D3D12_OBJECT(gpass_main_buffer.resource(), L"GPass Main Buffer");
	NAME_D3D12_OBJECT(gpass_depth_buffer.resource(), L"GPass Depth Buffer");

	return gpass_main_buffer.resource() && gpass_depth_buffer.resource();
}

bool create_gpass_pso_and_root_signature() {
	assert(!gpass_root_sig && !gpass_pso);

	// create GPass root signature
	D3DX::d3d12RootParameter parameters[1]{};
	parameters[0].as_constants(1, D3D12_SHADER_VISIBILITY_PIXEL, 1);
	const D3DX::d3d12RootSignatureDesc root_signature{ &parameters[0], _countof(parameters) };
	gpass_root_sig = root_signature.create();
	assert(gpass_root_sig);
	NAME_D3D12_OBJECT(gpass_root_sig, L"GPass Root Signature");

	// create GPass PSO
	struct {
		D3DX::d3d12PipelineStateSubobject_root_signature			root_signature{ gpass_root_sig };
		D3DX::d3d12PipelineStateSubobject_vs						vs{ SHADERS::get_engine_shader(SHADERS::engineShader::fullscreen_triangle_vs) };
		D3DX::d3d12PipelineStateSubobject_ps						ps{ SHADERS::get_engine_shader(SHADERS::engineShader::fill_color_ps) };
		D3DX::d3d12PipelineStateSubobject_primitive_topology		primitive_topology{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
		D3DX::d3d12PipelineStateSubobject_render_target_formats		render_target_formats;
		D3DX::d3d12PipelineStateSubobject_depth_stencil_format		depth_stencil_format{ depth_buffer_format };
		D3DX::d3d12PipelineStateSubobject_rasterizer				rasterizer{ D3DX::rasterizerState.no_cull };
		D3DX::d3d12PipelineStateSubobject_depth_stencil1			depth{ D3DX::depthState.disabled };
	} stream;

	// we can have 8 rts
	D3D12_RT_FORMAT_ARRAY rtf_array{};
	rtf_array.NumRenderTargets = 1;
	rtf_array.RTFormats[0] = main_buffer_format;

	stream.render_target_formats = rtf_array;

	gpass_pso = D3DX::create_pipeline_state(&stream, sizeof(stream));
	NAME_D3D12_OBJECT(gpass_pso, L"GPass Pipeline State Object");

	return gpass_root_sig && gpass_pso;
}

}

bool initialize() {
	// create the color buffer and depth buffer
	return create_buffers(initial_dimensions) && create_gpass_pso_and_root_signature();
}

void shutdown() {
	gpass_main_buffer.release();
	gpass_depth_buffer.release();
	dimensions = initial_dimensions;

	CORE::release(gpass_root_sig);
	CORE::release(gpass_pso);
}

// call this every frame before rendering anything to gpass
void set_size(MATH::u32v2 size) {
	MATH::u32v2& d{ dimensions };
	if (size.x > d.x || size.y > d.y)
	{
		d = { std::max(size.x, d.x), std::max(size.y, d.y)};
		create_buffers(d);
	}
}

void depth_prepass(id3d12GraphicsCommandList* cmd_list, const d3d12FrameInfo& info) {
	
}

void render(id3d12GraphicsCommandList* cmd_list, const d3d12FrameInfo& info) {
	cmd_list->SetGraphicsRootSignature(gpass_root_sig);
	cmd_list->SetPipelineState(gpass_pso);

	static u32 frame{ 0 };
	++frame;
	cmd_list->SetGraphicsRoot32BitConstant(0, frame, 0);

	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmd_list->DrawInstanced(3, 1, 0, 0);
}

void add_transitions_for_depth_prepass(D3DX::d3d12ResourceBarrier& barriers) {
	barriers.add(gpass_depth_buffer.resource(),
		D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void add_transitions_for_gpass(D3DX::d3d12ResourceBarrier& barriers) {
	barriers.add(gpass_main_buffer.resource(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	barriers.add(gpass_depth_buffer.resource(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void add_transitions_for_post_process(D3DX::d3d12ResourceBarrier& barriers) {
	barriers.add(gpass_main_buffer.resource(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}


void set_render_targets_for_depth_prepass(id3d12GraphicsCommandList* cmd_list) {
	const D3D12_CPU_DESCRIPTOR_HANDLE dsv{ gpass_depth_buffer.dsv() };
	cmd_list->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 0.0f, 0, 0, nullptr);
	cmd_list->OMSetRenderTargets(0, nullptr, 0, &dsv);
}

void set_render_targets_for_gpass(id3d12GraphicsCommandList* cmd_list) {
	const D3D12_CPU_DESCRIPTOR_HANDLE rtv{ gpass_main_buffer.rtv(0) };
	const D3D12_CPU_DESCRIPTOR_HANDLE dsv{ gpass_depth_buffer.dsv() };

	cmd_list->ClearRenderTargetView(rtv, clear_value, 0, nullptr);
	cmd_list->OMSetRenderTargets(1, &rtv, 0, &dsv);
}

}
