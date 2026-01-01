#pragma once
#include <filesystem>
#include "VulkanCommonHeaders.h"
#include <shaderc/shaderc.hpp>
// #include <glslang/Public/ShaderLang.h>
// #include <glslang/Public/ResourceLimits.h>
// #include <glslang/SPIRV/GlslangToSpv.h>
// #include <glslang/Include/ResourceLimits.h>
// #include <glslang/StandAlone/DirStackFileIncluder.h>

#if _DEBUG
#pragma comment(lib, "shaderc_combinedd.lib")
#else
#pragma comment(lib, "shaderc_combined.lib")
#endif

// #if _DEBUG
// #pragma comment(lib, "glslangd.lib")
// #pragma comment(lib, "SPIRVd.lib")
// #pragma comment(lib, "glslang-default-resource-limitsd.lib")
// #pragma comment(lib, "MachineIndependentd.lib")
// #pragma comment(lib, "GenericCodeGend.lib")
// #pragma comment(lib, "OSDependentd.lib")
// #pragma comment(lib, "SPIRV-Toolsd.lib")
// #pragma comment(lib, "SPIRV-Tools-optd.lib")
// #else
// #pragma comment(lib, "glslang.lib")
// #pragma comment(lib, "SPIRV.lib")
// #pragma comment(lib, "glslang-default-resource-limits.lib")
// #pragma comment(lib, "MachineIndependent.lib")
// #pragma comment(lib, "GenericCodeGen.lib")
// #pragma comment(lib, "OSDependent.lib")
// #pragma comment(lib, "SPIRV-Tools.lib")
// #pragma comment(lib, "SPIRV-Tools-opt.lib")
// #endif

namespace WAVEENGINE::GRAPHICS::VULKAN::SHADERS {

typedef struct compiledShader {
    u64         size;
    const u8*   byte_code;
} const *compiledShaderPtr;

struct shaderType {
    enum type : u8 {
        undefined = 0,
        vertex,
        tessellationControl,
        tessellationEvaluation,
        geometry,
        fragment,
        compute,
        task,
        mesh,

        rt,

        count,
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
    {"FillColor.hlsl", "FillColorPS", engineShader::id::fill_color_ps, shaderType::fragment},
    {"PostProcess.hlsl", "PostProcessPS", engineShader::id::post_process_ps, shaderType::fragment},
};

static_assert(_countof(shader_files) == engineShader::count);

class vulkanShader {
private:
    VkShaderModule                      _shaderModule{ VK_NULL_HANDLE };
    VkDevice                            _device{ VK_NULL_HANDLE };
    shaderType::type                    _stage{ shaderType::undefined };

public:
    vulkanShader() = default;
    vulkanShader(VkDevice device) : _device(device) {}
    vulkanShader(VkDevice device, VkShaderModuleCreateInfo& createInfo) : _device(device) {

    }
    vulkanShader(vulkanShader&& other) noexcept {
        VK_MOVE_PTR(_shaderModule);
        VK_MOVE_PTR(_device);
        _stage = other._stage;
        other._stage = shaderType::undefined;
    }

    VK_DEFINE_PTR_TYPE_OPERATOR(_shaderModule);
    VK_DEFINE_ADDRESS_FUNCTION(_shaderModule);

    VkShaderModule shaderModule() const { return _shaderModule; }
    shaderType::type stage() const { return _stage; }

    ~vulkanShader() {
        VK_DESTROY_PTR_BY(vkDestroyShaderModule, _device, _shaderModule);
    }

    // TODO
    VkResult create(VkShaderModuleCreateInfo& createInfo, engineShader::id shaderId);
};

class shaderIncluder : public shaderc::CompileOptions::IncluderInterface {
private:
    struct IncludeData {
        std::string* name;
        std::string* content;
    };

    std::filesystem::path _base_path;
public:
    explicit shaderIncluder(const std::filesystem::path& base_path) : _base_path(base_path) {}

    shaderc_include_result* GetInclude(const char* requested_source,
        shaderc_include_type type, const char* requesting_source, size_t include_depth) override;

    void ReleaseInclude(shaderc_include_result* data) override;
};

class vulkanShaderCompiler {
public:
    vulkanShaderCompiler() = default;
    ~vulkanShaderCompiler() = default;

    DISABLE_COPY_AND_MOVE(vulkanShaderCompiler);

    std::vector<u32> compile(const shaderFileInfo& info, const std::filesystem::path& full_path);
private:
    shaderc::Compiler               _compiler;
    shaderc::CompileOptions         _options;

    static shaderc_shader_kind get_shader_stage(shaderType::type type);
};

bool initialize();
void shutdown();

const std::vector<u32>* get_engine_shader(engineShader::id id);

}