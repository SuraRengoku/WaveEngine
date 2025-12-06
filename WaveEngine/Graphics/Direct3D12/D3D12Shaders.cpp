#include "d3d12Shaders.h"
#include "Content\ContentLoader.h"

namespace WAVEENGINE::GRAPHICS::D3D12::SHADERS {

namespace {

typedef struct compiledShader {
	u64			size;
	const u8*	byte_code;
} const *compiledShaderPtr; 

// each element in this array points to an offset withing the shaders blob.
compiledShaderPtr engine_shaders[engineShader::id::count]{};

/// <summary>
/// this is a chunk of memory that contains all compiled engine shaders.
/// The blob is an array of shader byte code consisting of an u64 size and an array of bytes.
/// </summary>
std::unique_ptr<u8[]> shaders_blob{};

bool load_engine_shaders() {
	assert(!shaders_blob);
	u64 size{ 0 };
	bool result{ CONTENT::load_engine_shaders(shaders_blob, size) };

	assert(shaders_blob && size);

	u64 offset{ 0 };
	u32 index{ 0 };
	while (offset < size && result) {
		assert(index < engineShader::id::count);
		compiledShaderPtr& shader{ engine_shaders[index] };
		assert(!shader);
		result &= index < engineShader::id::count && !shader;
		if (!result)	break;
		shader = reinterpret_cast<const compiledShaderPtr>(&shaders_blob[offset]);
		offset += sizeof(u64) + shader->size;
		++index;
	}
	assert(offset == size && index == engineShader::id::count);

	return true;
}

} // namespace anonymous

bool initialize() {
	return load_engine_shaders();
}

void shutdown() {
	for (u32 i{ 0 }; i < engineShader::id::count; ++i) {
		engine_shaders[i] = {};
	}
	shaders_blob.reset();
}

D3D12_SHADER_BYTECODE get_engine_shader(engineShader::id id) {
	assert(id < engineShader::id::count);
	const compiledShaderPtr shader{ engine_shaders[id] };
	assert(shader && shader->size);
	return { &shader->byte_code, shader->size };
}

} // namespace SHADERS
