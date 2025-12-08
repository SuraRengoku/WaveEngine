#pragma once
#include "D3D12CommonHeaders.h"

namespace WAVEENGINE::GRAPHICS::D3D12::SHADERS {

struct shaderType {
	enum type : u32 {
		vertex = 0,
		hull,
		domain,
		geometry,
		pixel,
		compute,
		amplification,
		mesh,

		count
	};
};

struct engineShader {
	enum id : u32 {
		fullscreen_triangle_vs = 0,
		fill_color_ps = 1,
		post_process_ps = 2,

		count
	};
};

bool initialize();
void shutdown();

D3D12_SHADER_BYTECODE get_engine_shader(engineShader::id id);


}
