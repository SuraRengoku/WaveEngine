#include "D3D12PostProcess.h"
#include "D3D12Surface.h"
#include "D3D12GPass.h"

namespace WAVEENGINE::GRAPHICS::D3D12::POSTP {

namespace {

struct ppRootParamIndices {
	enum : u32 {
		root_constants,
		descriptor_table,

		count
	};
};

ID3D12RootSignature*			pp_root_sig{ nullptr };
ID3D12PipelineState*			pp_pso{nullptr};

bool create_pp_pso_and_root_signature() {
	assert(!pp_root_sig && !pp_pso);

	D3DX::d3d12DescriptorRange range{
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND, 0, 0,
		D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE
	};

	// create FX root signature
	using idx = ppRootParamIndices;
	D3DX::d3d12RootParameter parameters[idx::count]{};
	parameters[idx::root_constants].as_constants(1, D3D12_SHADER_VISIBILITY_PIXEL, 1);
	parameters[idx::descriptor_table].as_descriptor_table(D3D12_SHADER_VISIBILITY_PIXEL, &range, 1);
	const D3DX::d3d12RootSignatureDesc root_signature{ &parameters[0], _countof(parameters) };
	pp_root_sig = root_signature.create();
	assert(pp_root_sig);
	NAME_D3D12_OBJECT(pp_root_sig, L"Post-process FX Root Signature");

	// create FX PSO
	struct {
		D3DX::d3d12PipelineStateSubobject_root_signature			root_signature{ pp_root_sig };
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

	pp_pso = D3DX::create_pipeline_state(&stream, sizeof(stream));
	NAME_D3D12_OBJECT(pp_pso, L"Post-process FX Pipeline State Object");

	return pp_root_sig && pp_pso;
}

}

bool initialize() {
	return create_pp_pso_and_root_signature();
}

void shutdown() {


	CORE::release(pp_root_sig);
	CORE::release(pp_pso);
}

void post_process_render(id3d12GraphicsCommandList* cmd_list, D3D12_CPU_DESCRIPTOR_HANDLE target_rtv) {
	cmd_list->SetGraphicsRootSignature(pp_root_sig);
	cmd_list->SetPipelineState(pp_pso);

	using idx = ppRootParamIndices;
	cmd_list->SetGraphicsRoot32BitConstant(idx::root_constants, GPASS::main_buffer().srv().index, 0);
	cmd_list->SetGraphicsRootDescriptorTable(idx::descriptor_table, CORE::srv_heap().gpu_start());

	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// NOTE: we do not have to clear the render target, because each pixel will be overwritten by pixels from gpass main buffer
	//		 we also do not need a depth buffer.
	cmd_list->OMSetRenderTargets(1, &target_rtv, 1, nullptr);
	cmd_list->DrawInstanced(3, 1, 0, 0);
}

}
