#include "VulkanShader.h"
#include "Content\ContentLoader.h"

namespace WAVEENGINE::GRAPHICS::VULKAN::SHADERS {

// compiledShaderPtr engine_shaders[engineShader::id::count]{};

namespace {

std::array<std::vector<u32>, engineShader::id::count> engine_shaders;

std::unique_ptr<u8[]> shaders_blob{};

bool load_engine_shaders() {
    auto engine_shades_path = get_engine_shaders_path();
    if (!std::filesystem::exists(engine_shades_path)) return false;

    std::ifstream file(engine_shades_path, std::ios::in | std::ios::binary);
    if (!file) return false;

    u32 index = 0;
    while (file.peek() != EOF && index < engineShader::id::count) {
        // read u64
        size_t byte_size = 0;
        file.read(reinterpret_cast<char*>(&byte_size), sizeof(byte_size));

        if (byte_size == 0) break;

        // compute # of u32
        size_t u32_count = byte_size / sizeof(u32);
        engine_shaders[index].resize(u32_count);
        file.read(reinterpret_cast<char*>(engine_shaders[index].data()), byte_size);
        ++index;
    }
    file.close();
    return index == engineShader::id::count;
}

// for specific memory layout
bool load_engine_shaders_raw() {
    assert(!shaders_blob);
    u64 size{ 0 };
    bool result{ CONTENT::load_engine_shaders(shaders_blob, size) };

    assert(shaders_blob && size);

    u64 offset{ 0 };
    u32 index{ 0 };
    while (offset < size && result) {
        assert(index < engineShader::id::count);
        result &= index < engineShader::id::count;
        if (!result) break;

        // read size
        size_t byte_size = *reinterpret_cast<size_t*>(&shaders_blob[offset]);
        offset += sizeof(size_t);

        // load u32 to vector
        size_t u32_count = byte_size / sizeof(u32);
        engine_shaders[index].assign(
            reinterpret_cast<u32*>(&shaders_blob[offset]),
            reinterpret_cast<u32*>(&shaders_blob[offset] + byte_size)
        );

        offset += byte_size;
        ++index;
    }
    assert(offset == size && index == engineShader::id::count);

    return result;
}

const std::vector<u32>* get_engine_shader(engineShader::id id) {
    assert(id < engineShader::id::count);
    if (engine_shaders[id].empty()) {
        return nullptr;
    }
    return &engine_shaders[id];
}

}

bool loadEngineShaders() {
    return load_engine_shaders();
}

void unloadEngineShaders() {
    for (auto& shader : engine_shaders) {
        shader.clear();
    }
    shaders_blob.reset();
}

VkResult vulkanShader::create(VkShaderModuleCreateInfo& createInfo, engineShader::id shaderId) {
    auto* spir_v = get_engine_shader(shaderId);
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spir_v->size() * sizeof(u32);
    createInfo.pCode = spir_v->data();

    if (VkResult result = vkCreateShaderModule(_device, &createInfo, nullptr, &_shaderModule)) {
        debug_error("::VULKAN:ERROR Failed to create shader\n");
        return result;
    }

    return VK_SUCCESS;
}

shaderc_include_result* shaderIncluder::GetInclude(const char* requested_source, shaderc_include_type type,
                                                   const char* requesting_source, size_t include_depth) {

    std::filesystem::path include_path = _base_path / requested_source;;

    std::string debug_msg = "Looking for include: " + include_path.string() + "\n";

    debug_output(debug_msg.c_str());

    auto* result = new shaderc_include_result;
    std::ifstream file(include_path, std::ios::binary);

    if (!file.is_open()) {
        std::string error_msg = "Failed to open include file: " + include_path.string() + "\n";

        debug_error(error_msg.c_str());

        auto* error_content = new std::string(error_msg);
        result->source_name = "";
        result->source_name_length = 0;
        result->content = error_content->c_str();
        result->content_length = error_content->size();
        result->user_data = new IncludeData{ nullptr, error_content };
        return result;
    }

    auto* content = new std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    auto* name = new std::string(requested_source);

    std::string success_msg = "Successfully loaded: " + include_path.string() +
                                     " (" + std::to_string(content->size()) + " bytes)\n";
    debug_output(success_msg.c_str());

    result->source_name = name->c_str();
    result->source_name_length = name->size();
    result->content = content->c_str();
    result->content_length = content->size();
    result->user_data = new IncludeData{ name, content };

    return result;
}

void shaderIncluder::ReleaseInclude(shaderc_include_result* data) {
    if (data->user_data) {
        auto* include_data = static_cast<IncludeData*>(data->user_data);
        delete include_data->name;
        delete include_data->content;
        delete include_data;
    }
    delete data;
}

std::vector<u32> vulkanShaderCompiler::compile(const shaderFileInfo& info, const std::filesystem::path& full_path) const {
    // read hlsl
    std::ifstream file(full_path);
    if (!file) {
        debug_error("Failed to open shader file\n");
        return {};
    }

    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // configure compilation options
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
    options.SetTargetSpirv(shaderc_spirv_version_1_5);
    options.SetSourceLanguage(shaderc_source_language_hlsl);
    options.SetHlslIoMapping(true);

    // set includer
    options.SetIncluder(std::make_unique<shaderIncluder>(full_path.parent_path()));

#if _DEBUG
    options.SetOptimizationLevel(shaderc_optimization_level_zero);
    options.SetGenerateDebugInfo();
#else
    options.SetOptimizationLevel(shaderc_optimization_level_performance);
#endif

    // compile
    shaderc_shader_kind kind = get_shader_stage(info.type);
    shaderc::SpvCompilationResult result = _compiler.CompileGlslToSpv(
        source,
        kind,
        full_path.string().c_str(),
        info.function, // entry point
        options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::string error = "Shader compilation failed:\n" + result.GetErrorMessage();
        debug_error(error.c_str());
        return {};
    }

    return { result.cbegin(), result.cend() };
}

shaderc_shader_kind vulkanShaderCompiler::get_shader_stage(shaderType::type type) {
    switch (type) {
        case shaderType::vertex:                    return shaderc_vertex_shader;
        case shaderType::tessellationControl:       return shaderc_tess_control_shader;
        case shaderType::tessellationEvaluation:    return shaderc_tess_evaluation_shader;
        case shaderType::geometry:                  return shaderc_geometry_shader;
        case shaderType::fragment:                  return shaderc_fragment_shader;
        case shaderType::compute:                   return shaderc_compute_shader;
        case shaderType::task:                      return shaderc_task_shader;
        case shaderType::mesh:                      return shaderc_mesh_shader;
        default:                                    return shaderc_glsl_infer_from_source;
    }
}

}
