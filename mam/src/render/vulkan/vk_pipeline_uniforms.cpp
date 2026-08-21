#include "render/vulkan/vk_pipeline.hpp"
#include "render/vulkan/vk_device.hpp"
#include "render/vulkan/vk_texture.hpp"

namespace mam {

  int VKPipeline::getUniformLocation(const char* name) const {
    auto it = uniforms.find(name);
    if (it == uniforms.end()) return -1;
    return it->second.offset;
  }

  void VKPipeline::setUniform(const char* name, const void* value) {
    auto it = uniforms.find(name);
    if (it == uniforms.end()) return;

    const VKUniformInfo& info = it->second;
    if (mappedUniforms_.size() < uniformBufferSize) {
      mappedUniforms_.resize(static_cast<size_t>(uniformBufferSize));
    }
    std::memcpy(mappedUniforms_.data() + info.offset, value, info.size);
  }

  void VKPipeline::setSamplerUniform(const char* name, int unit) {}

  void VKPipeline::createUniformBuffer() {
    uniforms.clear();

    CreateUniform("u_view", offsetof(UniformBufferObject, view), sizeof(glm::mat4));
    CreateUniform("u_model", offsetof(UniformBufferObject, model), sizeof(glm::mat4));
    CreateUniform("u_projection", offsetof(UniformBufferObject, proj), sizeof(glm::mat4));

    CreateUniform("u_viewPos", offsetof(UniformBufferObject, viewPos), sizeof(glm::vec3));
    CreateUniform("u_lightCount", offsetof(UniformBufferObject, lightCount), sizeof(int));
    CreateUniform("u_baseColor", offsetof(UniformBufferObject, baseColor), sizeof(glm::vec3));
    CreateUniform("u_metallic", offsetof(UniformBufferObject, metallic), sizeof(float));
    CreateUniform("u_roughness", offsetof(UniformBufferObject, roughness), sizeof(float));

    CreateUniform("u_useAlbedoMap", offsetof(UniformBufferObject, useAlbedoMap), sizeof(int));
    CreateUniform("u_useNormalMap", offsetof(UniformBufferObject, useNormalMap), sizeof(int));
    CreateUniform("u_useRoughnessMap", offsetof(UniformBufferObject, useRoughnessMap), sizeof(int));
    CreateUniform("u_useMetallicMap", offsetof(UniformBufferObject, useMetallicMap), sizeof(int));

    uniformBufferSize = sizeof(UniformBufferObject);

    mappedUniforms_.clear();
    mappedUniforms_.resize(static_cast<size_t>(uniformBufferSize), 0);

  }

  void VKPipeline::CreateUniform(std::string name, uint32_t offset, uint32_t size) {
    uniforms[name] = { offset, size };
  }



}