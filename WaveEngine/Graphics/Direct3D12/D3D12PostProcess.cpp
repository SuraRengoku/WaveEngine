#include "D3D12PostProcess.h"

namespace WAVEENGINE::GRAPHICS::D3D12::FX {

namespace {

ID3D12RootSignature*			fx_root_sig{ nullptr };
ID3D12PipelineState*			fx_pso{nullptr};

bool create_fx_pso_and_root_signature() {
	assert(!fx_root_sig && !fx_pso);

	// create FX root signature
	D3DX::d3d12RootParameter parameters[1]{};
	parameters[0].as_constants(1, D3D12_SHADER_VISIBILITY_PIXEL, 1);
	const D3DX::d3d12RootSignatureDesc root_signature{ &parameters[0], _countof(parameters) };
	fx_root_sig = root_signature.create();
	assert(fx_root_sig);
	NAME_D3D12_OBJECT(fx_root_sig, L"Post-process FX Root Signature");

	// create FX PSO
	struct {
		D3DX::d3d12PipelineStateSubobject_root_signature			root_signature{ fx_root_sig };
		D3DX::d3d12PipelineStateSubobject_vs						vs{ SHADERS::get_engine_shader(SHADERS::engineShader::fullscreen_triangle_vs) };
		D3DX::d3d12PipelineStateSubobject_ps						ps{ SHADERS::get_engine_shader(SHADERS::engineShader::post_process_ps) };
		D3DX::d3d12PipelineStateSubobject_primitive_topology		primitive_topology{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
		D3DX::d3d12PipelineStateSubobject_render_target_formats		render_target_formats;
		D3DX::d3d12PipelineStateSubobject_rasterizer				rasterizer{ D3DX::rasterizerState.no_cull };
	} stream;

	D3D12_RT_FORMAT_ARRAY rtf_array{};
	rtf_array.NumRenderTargets = 1;
	rtf_array.RTFormats[0] = d3d12Surface::default_back_buffer_format;

	stream.render_target_formats = rtf_array;

	fx_pso = D3DX::create_pipeline_state(&stream, sizeof(stream));
	NAME_D3D12_OBJECT(fx_pso, L"Post-process FX Pipeline State Object");

	return fx_root_sig && fx_pso;
}

}

bool initialize() {
	return create_fx_pso_and_root_signature();
}

void shutdown() {


	CORE::release(fx_root_sig);
	CORE::release(fx_pso);
}
}
