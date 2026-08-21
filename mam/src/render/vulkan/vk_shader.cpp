#include "render/vulkan/vk_shader.hpp"
#include "render/vulkan/vk_device.hpp"
#include "render/vulkan/vk_context.hpp"

#include <shaderc/shaderc.hpp>
#include <iostream>

static std::vector<char> readFile(const std::string& filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file!");
  }

  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();

  return buffer;
}

namespace mam {

  VKShader::VKShader(VKDevice* device)
  {
    vk_device_ = device;
    is_compiled_ = false;
    type_ = Shader::Invalid;
    id_ = 0;
  }

  VKShader::~VKShader()
  {
    if (shaderModule_) {
      vk_device_->device->destroyShaderModule(shaderModule_);
      shaderModule_ = nullptr;
    }
  }

  void VKShader::loadSource(const Type shader_type,
                                 const char *source,
                                 const unsigned int source_size)
  {
    type_ = shader_type;
    source_.assign(source, source + source_size);
    is_compiled_ = false;
  }

  void VKShader::loadSourceFromFile(const Type shader_type, const char *path)
  {
    /*
    type_ = shader_type;
    auto shader_code = readFile(path);
    shaderModule_ = vk_device_->createShaderModule(shader_code);
    is_compiled_ = true;

    vk::ShaderStageFlagBits flag = vk::ShaderStageFlagBits::eFragment;
    switch (type_)
    {
    case mam::Shader::Invalid:
      is_compiled_ = false;
      return;
      break;
    case mam::Shader::Fragment:
      flag = vk::ShaderStageFlagBits::eFragment;
      break;
    case mam::Shader::Vertex:
      flag = vk::ShaderStageFlagBits::eVertex;
      break;
    case mam::Shader::Geometry:
      flag = vk::ShaderStageFlagBits::eGeometry;
      break;
    }

    shaderStage_ = {
        vk::PipelineShaderStageCreateFlags(),
        flag,
        shaderModule_,
        "main"
    };
    */
    type_ = shader_type;

    auto shader_code = readFile(path);

    source_.assign(shader_code.begin(), shader_code.end());

    if (shaderModule_) {
      vk_device_->device->destroyShaderModule(shaderModule_);
      shaderModule_ = nullptr;
    }

    is_compiled_ = false;
  }

  bool VKShader::compile()
  {
    if (source_.empty()) {
      std::cerr << "VKShader::compile() failed: empty shader source\n";
      is_compiled_ = false;
      return false;
    }

    shaderc_shader_kind kind = shaderc_glsl_infer_from_source;

    vk::ShaderStageFlagBits vkStage = vk::ShaderStageFlagBits::eFragment;

    switch (type_)
    {
    case mam::Shader::Vertex:
      kind = shaderc_vertex_shader;
      vkStage = vk::ShaderStageFlagBits::eVertex;
      break;

    case mam::Shader::Fragment:
      kind = shaderc_fragment_shader;
      vkStage = vk::ShaderStageFlagBits::eFragment;
      break;

    case mam::Shader::Geometry:
      kind = shaderc_geometry_shader;
      vkStage = vk::ShaderStageFlagBits::eGeometry;
      break;

    case mam::Shader::Invalid:
    default:
      std::cerr << "VKShader::compile() failed: invalid shader type\n";
      is_compiled_ = false;
      return false;
    }

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    options.AddMacroDefinition("MAM_VULKAN", "1");

    options.SetTargetEnvironment(
      shaderc_target_env_vulkan,
      shaderc_env_version_vulkan_1_0
    );

    options.SetSourceLanguage(shaderc_source_language_glsl);
    options.SetOptimizationLevel(shaderc_optimization_level_zero);

    shaderc::SpvCompilationResult result =
      compiler.CompileGlslToSpv(
        source_,
        kind,
        "mam_dynamic_shader",
        options
      );

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
      std::cerr << "VKShader GLSL -> SPIR-V compile error:\n"
        << result.GetErrorMessage()
        << "\n\nSOURCE:\n"
        << source_
        << std::endl;

      is_compiled_ = false;
      return false;
    }

    std::vector<uint32_t> spirv(result.cbegin(), result.cend());

    if (shaderModule_) {
      vk_device_->device->destroyShaderModule(shaderModule_);
      shaderModule_ = nullptr;
    }

    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.codeSize = spirv.size() * sizeof(uint32_t);
    createInfo.pCode = spirv.data();

    try {
      shaderModule_ = vk_device_->device->createShaderModule(createInfo);
    }
    catch (vk::SystemError&) {
      std::cerr << "VKShader::compile() failed: could not create shader module\n";
      is_compiled_ = false;
      return false;
    }

    shaderStage_ = {
      vk::PipelineShaderStageCreateFlags(),
      vkStage,
      shaderModule_,
      "main"
    };

    spirvCache_ = spirv;

    is_compiled_ = true;
    return true;
  }

  bool VKShader::is_compiled() const
  {
    return is_compiled_;
  }

  Shader::Type VKShader::type() const
  {
    return type_;
  }

  uint32_t VKShader::id() const
  {
    return id_;
  }
}

