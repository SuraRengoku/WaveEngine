#include "ShaderCompilation.h"

#include "Graphics\Direct3D12\D3D12Core.h"
#include "Graphics\Direct3D12\D3D12Shaders.h"
#include "Graphics\Vulkan\VulkanShader.h"

using namespace WAVEENGINE;
using namespace WAVEENGINE::GRAPHICS;

namespace {

#if USE_VULKAN || USE_D3D12 || USE_D3D11
constexpr const char* shaders_source_path{ "../../WaveEngine/Graphics/Shaders/HLSL/" };
#elif USE_OPENGL
constexpr const char* shaders_source_path{ "../../WaveEngine/Graphics/Shaders/GLSL/" };
#endif

decltype(auto) get_engine_shaders_path() {
#if USE_D3D12
	return std::filesystem::absolute(GRAPHICS::get_engine_shaders_path(GRAPHICS::graphics_platform::Direct3D12));
#elif USE_D3D11
	return std::filesystem::absolute(GRAPHICS::get_engine_shaders_path(GRAPHICS::graphics_platform::Direct3D11));
#elif USE_VULKAN
	return std::filesystem::absolute(GRAPHICS::get_engine_shaders_path(GRAPHICS::graphics_platform::Vulkan));
#elif USE_OPENGL
	return std::filesystem::absolute(GRAPHICS::get_engine_shaders_path(GRAPHICS::graphics_platform::OpenGL));
#endif
}

bool compiled_shaders_are_up_to_date() {
	auto engine_shaders_path = get_engine_shaders_path();
	if (!std::filesystem::exists(engine_shaders_path)) return false;
	auto shaders_compilation_time = std::filesystem::last_write_time(engine_shaders_path);

	std::filesystem::path path{};
	std::filesystem::path full_path{};

	// check if either of the engine shader source files is newer than the compiled shader file.
	// In that case, we need to recompile.
#if USE_D3D12
	for (u32 i{ 0 }; i < D3D12::SHADERS::engineShader::id::count; ++i) {
		auto& info = D3D12::SHADERS::shader_files[i];
#elif USE_D3D11
#elif USE_VULKAN
	for (u32 i{0}; i < VULKAN::SHADERS::engineShader::id::count; ++i) {
		auto& info = VULKAN::SHADERS::shader_files[i];
#elif USE_OPENGL

#endif

		path = shaders_source_path;
		path += info.file;
		full_path = std::filesystem::absolute(path);
		if (!std::filesystem::exists(full_path)) return false; 

		auto shader_file_time = std::filesystem::last_write_time(full_path);
		if (shader_file_time > shaders_compilation_time)
			return false;
	}

	return true;
}

// data structure in memory
// [size_t: shader0_byte_size][u32[]: shader0_data]
// [size_t: shader1_byte_size][u32[]: shader1_data]
#if USE_D3D12 || USE_D3D11
bool save_compiled_shaders(UTL::vector<ComPtr<IDxcBlob>>& shaders) {
	auto engine_shaders_path = get_engine_shaders_path();
	std::filesystem::create_directories(engine_shaders_path.parent_path());
	std::ofstream file(engine_shaders_path, std::ios::out | std::ios::binary);
	if (!file || !std::filesystem::exists(engine_shaders_path)) {
		file.close();
		return false;
	}

	for (auto& shader : shaders) {
		const D3D12_SHADER_BYTECODE byte_code{ shader->GetBufferPointer(), shader->GetBufferSize() };
		file.write((char*)&byte_code.BytecodeLength, sizeof(byte_code.BytecodeLength));
		file.write((char*)byte_code.pShaderBytecode, byte_code.BytecodeLength);
	}

	file.close();
	return true;
}
#elif USE_VULKAN
bool save_compiled_shaders(UTL::vector<std::vector<u32>>& shaders) {
	auto engine_shaders_path = get_engine_shaders_path();
	std::filesystem::create_directories(engine_shaders_path.parent_path());
	std::ofstream file(engine_shaders_path, std::ios::out | std::ios::binary);
	if (!file) return false;

	for (auto& spir_v : shaders) {
		size_t size = spir_v.size() * sizeof(u32); // size -> u64
		file.write(reinterpret_cast<char*>(&size), sizeof(size));
		file.write(reinterpret_cast<char*>(spir_v.data()), size);
	}

	file.close();
	return true;
}
#elif USE_OPENGL

#endif

} // namespace anonymous

#if _WIN32
#if USE_D3D12 || USE_D3D11
bool compile_shaders() {
	if (compiled_shaders_are_up_to_date()) return true;
	UTL::vector<ComPtr<IDxcBlob>> shaders; // after compilation the shader bytecode will be returned in an IDxcBlob interface.
	
	std::filesystem::path path{};
	std::filesystem::path full_path{};

	D3D12::SHADERS::shaderCompiler compiler{};
	// compile shaders and them together in a buffer in the same order of compilation.
	for (u32 i{ 0 }; i < D3D12::SHADERS::engineShader::id::count; ++i) {
		auto& info = D3D12::SHADERS::shader_files[i];
		path = shaders_source_path;
		path += info.file;
		full_path = std::filesystem::absolute(path);
		if (!std::filesystem::exists(full_path)) return false;

		ComPtr<IDxcBlob> compiled_shader{ compiler.compile(info, full_path) };
		if (compiled_shader && compiled_shader->GetBufferPointer() && compiled_shader->GetBufferSize()) {
			shaders.emplace_back(std::move(compiled_shader));
		}
		else {
			return false;
		}
	}

	return save_compiled_shaders(shaders);
}
#elif USE_VULKAN
bool compile_shaders() {
	if (compiled_shaders_are_up_to_date()) return true;

	UTL::vector<std::vector<u32>> shaders;
	VULKAN::SHADERS::vulkanShaderCompiler compiler;

	for (u32 i{0}; i < VULKAN::SHADERS::engineShader::count; ++i) {
		auto& info = VULKAN::SHADERS::shader_files[i];
		std::filesystem::path path = shaders_source_path;
		path += info.file;
		auto full_path = std::filesystem::absolute(path);

		if (!std::filesystem::exists(full_path)) return false;

		// i = 1 error
		auto spir_v = compiler.compile(info, full_path);
		if (spir_v.empty()) return false;

		shaders.emplace_back(std::move(spir_v));
	}

	return save_compiled_shaders(shaders);
}
#elif USE_OPENGL

#endif

#endif