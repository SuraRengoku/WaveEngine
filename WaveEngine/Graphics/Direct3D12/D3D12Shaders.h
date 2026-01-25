#pragma once
#include "D3D12CommonHeaders.h"
// before include dxcapi.h we should first include windows related files
#include "..\packages\DirectXShaderCompiler\inc\dxcapi.h"
#include "..\packages\DirectXShaderCompiler\inc\d3d12shader.h"
//#include <dxcapi.h>
//#include <d3d12shader.h>

// add DXC lib link, we wouldn't need to do this if DXC had a NuGet package
#pragma comment(lib, "../packages/DirectXShaderCompiler/lib/x64/dxcompiler.lib")

// ref: https://computergraphics.stackexchange.com/questions/9953/dxc-error-when-compiling-pso
// copy dxcompiler.dll and dxil.dll into output dir to avoid Vertex Shader error

#include <filesystem>

#if _WIN32
using namespace Microsoft::WRL;
#endif

namespace WAVEENGINE::GRAPHICS::D3D12::SHADERS {

constexpr const char* shaders_source_path{ "../../WaveEngine/Graphics/Shaders/HLSL/" };

struct shaderType {
	enum type : u32 {
		undefined = 0,
		vertex,
		hull,
		domain,
		geometry,
		pixel,
		compute,
		amplification,
		mesh,

		rt,

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

struct shaderFileInfo {
	const char*			file;
	const char*			function;
	engineShader::id	id;
	shaderType::type	type;
};
constexpr shaderFileInfo shader_files[]{
	{"FullScreenTriangle.hlsl", "FullScreenTriangleVS", engineShader::id::fullscreen_triangle_vs, shaderType::vertex},
	{"FillColor.hlsl", "FillColorPS", engineShader::id::fill_color_ps, shaderType::pixel},
	{"PostProcess.hlsl", "PostProcessPS", engineShader::id::post_process_ps, shaderType::pixel},
};

static_assert(_countof(shader_files) == engineShader::count);

bool initialize();
void shutdown();

D3D12_SHADER_BYTECODE get_engine_shader(engineShader::id id);

inline std::wstring to_wstring(const char* c) {
	std::string s{ c };
	return { s.begin(), s.end() };
}

class shaderCompiler {
public:
	shaderCompiler() {
		HRESULT hr{ S_OK };
		DXCall(hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&_compiler)));
		if (FAILED(hr)) return;
		DXCall(hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&_utils)));
		if (FAILED(hr)) return;
		DXCall(hr = _utils->CreateDefaultIncludeHandler(&_include_handler));
		if (FAILED(hr)) return;
	}

	DISABLE_COPY_AND_MOVE(shaderCompiler);

	IDxcBlob* compile(shaderFileInfo info, std::filesystem::path full_path) {
		assert(_compiler && _utils && _include_handler);
		HRESULT hr{ S_OK };

		// load the source file using Utils interface.
		ComPtr<IDxcBlobEncoding> source_blob{ nullptr };
		DXCall(hr = _utils->LoadFile(full_path.c_str(), nullptr, &source_blob));
		if (FAILED(hr))	return nullptr;
		assert(source_blob && source_blob->GetBufferSize());

		std::wstring file{ to_wstring(info.file) };
		std::wstring func{ to_wstring(info.function) };
		std::wstring prof{ to_wstring(_profile_strings[(u32)info.type]) };
		std::wstring inc{ to_wstring(shaders_source_path) };

		LPCWSTR args[]{
			file.c_str(),				// optional shader source file name for error reporting
			L"-E", func.c_str(),		// entry function
			L"-T", prof.c_str(),		// target profile
			L"-I", inc.c_str(),			// include path
			DXC_ARG_ALL_RESOURCES_BOUND,
#if _DEBUG
			DXC_ARG_DEBUG,
			DXC_ARG_SKIP_OPTIMIZATIONS,
#else
			DXC_ARG_OPTIMIZATION_LEVEL3,
#endif
			DXC_ARG_WARNINGS_ARE_ERRORS,
			L"-Qstrip_reflect",			// strip reflections into a separate blob
			L"-Qstrip_debug",			// strip debug information into a separate blob
		};

		OutputDebugStringA("Compiling ");
		OutputDebugStringA(info.file);

		return compile(source_blob.Get(), args, _countof(args));
	}

	IDxcBlob* compile(IDxcBlobEncoding* source_blob, LPCWSTR* args, u32 num_args) {
		DxcBuffer buffer{};
		buffer.Encoding = DXC_CP_ACP; // auto-detect text format
		buffer.Ptr = source_blob->GetBufferPointer();
		buffer.Size = source_blob->GetBufferSize();

		HRESULT hr{ S_OK };
		ComPtr<IDxcResult> results{ nullptr };
		DXCall(hr = _compiler->Compile(&buffer, args, num_args, _include_handler.Get(), IID_PPV_ARGS(&results)));
		if (FAILED(hr)) return nullptr;

		ComPtr<IDxcBlobUtf8> errors{ nullptr };
		DXCall(hr = results->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr));
		if (FAILED(hr)) return nullptr;

		if (errors && errors->GetStringLength()) {
			OutputDebugStringA("\nShader Compilation Error: \n");
			OutputDebugStringA(errors->GetStringPointer());
		} else {
			OutputDebugStringA("[ Succeed ]");
		}
		OutputDebugStringA("\n");

		HRESULT status{S_OK};
		DXCall(hr = results->GetStatus(&status));
		if (FAILED(hr) || FAILED(status)) return nullptr;

		ComPtr<IDxcBlob> shader{ nullptr };
		DXCall(hr = results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), nullptr));
		if (FAILED(hr)) return nullptr;

		return shader.Detach();
	}

private:
	// NOTE: Shader model 6.x can also be used (AS and MS are only supported from SM6.5 on).
	constexpr static const char*	_profile_strings[shaderType::type::count]{ "undefined", "vs_6_5", "hs_6_5", "ds_6_5", "gs_6_5", "ps_6_5", "cs_6_5", "as_6_5", "ms_6_5", "rt" };
	static_assert(_countof(_profile_strings) == shaderType::type::count);

	ComPtr<IDxcCompiler3>			_compiler{ nullptr };
	ComPtr<IDxcUtils>				_utils{ nullptr };
	ComPtr<IDxcIncludeHandler>		_include_handler{ nullptr };
};

}
