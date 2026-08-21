
#include "render/api/forward_pass.hpp"

#include "core/camera.hpp"
#include "render/api/graphics_device.hpp"
#include "render/api/renderer.hpp"
#include "render/api/mesh.hpp"
#include "matsys/material.hpp"
#include "render/api/pipeline.hpp"
#include "render/api/texture.hpp"
#include "render/api/buffer.hpp"

#include "render/vulkan/vk_buffer.hpp"
#include "render/vulkan/vk_device.hpp"
#include "render/vulkan/vk_pipeline.hpp"
#include "render/vulkan/vk_texture.hpp"
#include "render/vulkan/vk_framebuffer.hpp"

#include "ecs/light_probe.hpp"
#include "ecs/system.hpp"

#include "ui/imgui_wrapper.hpp"

#include <limits>
#include <cstdio>

namespace mam {

#pragma region HELPERS

  static glm::vec3 getObjectWorldPosition(const glm::mat4& model)
  {
    return glm::vec3(model[3]);
  }

  static bool computeBlendedProbeSH(
    const ProbeList* probeList,
    const glm::vec3& objectPos,
    glm::vec3 outSH[kSHCoeffCount])
  {
    if (!probeList || probeList->probes.empty())
      return false;

    for (int i = 0; i < kSHCoeffCount; ++i)
      outSH[i] = glm::vec3(0.0f);

    float totalWeight = 0.0f;

    for (const auto& probe : probeList->probes) {
      const glm::vec3 probePos = glm::vec3(probe.position_radius);
      const float radius = probe.position_radius.w;

      if (radius <= 0.001f)
        continue;

      const float dist = glm::distance(objectPos, probePos);

      if (dist > radius)
        continue;

      float w = 1.0f - glm::clamp(dist / radius, 0.0f, 1.0f);
      w = w * w;

      for (int c = 0; c < kSHCoeffCount; ++c)
        outSH[c] += glm::vec3(probe.sh[c]) * w;

      totalWeight += w;
    }

    if (totalWeight > 0.0001f) {
      for (int c = 0; c < kSHCoeffCount; ++c)
        outSH[c] /= totalWeight;

      return true;
    }

    const GPUProbe* nearest = nullptr;
    float bestDist = std::numeric_limits<float>::max();

    for (const auto& probe : probeList->probes) {
      const float dist = glm::distance(objectPos, glm::vec3(probe.position_radius));

      if (dist < bestDist) {
        bestDist = dist;
        nearest = &probe;
      }
    }

    if (!nearest)
      return false;

    for (int c = 0; c < kSHCoeffCount; ++c)
      outSH[c] = glm::vec3(nearest->sh[c]);

    return true;
  }

  static void uploadProbeSHForCommand(
    const RenderPassContext& ctx,
    Material* material,
    const glm::mat4& model)
  {
    if (!material) return;

    ProbeList* probeList = nullptr;

    if (ctx.appContext)
      probeList = ctx.appContext->Get<ProbeList>();

    glm::vec3 blendedSH[kSHCoeffCount];
    const glm::vec3 objectPos = getObjectWorldPosition(model);

    if (!computeBlendedProbeSH(probeList, objectPos, blendedSH)) {
      material->setParameter("u_probeEnabled", 0);
      return;
    }

    char uniformName[32];

    for (int i = 0; i < kSHCoeffCount; ++i) {
      std::snprintf(uniformName, sizeof(uniformName), "u_sh[%d]", i);
      material->setParameter(uniformName, blendedSH[i]);
    }

    material->setParameter("u_probeEnabled", 1);
  }

#pragma endregion

    void ForwardPass::init(u32 width, u32 height) {

        FrameBufferSpec spec;
        spec.width        = width;
        spec.height       = height;
        spec.colorFormats = { Format::RGBA8 };
        spec.hasDepth     = true;
        spec.depthFormat  = Format::DEPTH24STENCIL8;

        framebuffer_ = gd_.createFrameBuffer(spec);
        assert(framebuffer_ && "ForwardPass: failed to create framebuffer");
        
        auto* vkdev = dynamic_cast<VKDevice*>(&gd_);

        if (vkdev) {
          vkdev->setCurrentRenderPass(
            static_cast<VKFramebuffer*>(framebuffer_.get())->renderPass()
          );

          vkdev->setCurrentColorAttachmentCount(
            static_cast<u32>(spec.colorFormats.size())
          );
        }

        lightBuffer_ = gd_.createShaderStorageBuffer(0);
    }

    void ForwardPass::resize(u32 width, u32 height) {
        if (framebuffer_)
            framebuffer_->resize(width, height);
    }

    void ForwardPass::execute(const RenderPassContext& ctx) {
      uploadLights(ctx);

      framebuffer_->bind();
      gd_.enableDepthTest();

      if (lightingOutput_) {
        u32 w = framebuffer_->spec().width;
        u32 h = framebuffer_->spec().height;

        glBindFramebuffer(GL_READ_FRAMEBUFFER, lightingOutput_->id());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer_->id());
        glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        if (gBuffer_) {
          glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer_->id());
          glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer_->id());
          glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_->id());

        drawOpaqueInstanced(ctx);
        drawSkybox(ctx);
      } else {
        gd_.setClearColor(0.3f, 0.3f, 0.3f, 1.f);
        gd_.clearBuffers();
        drawOpaque(ctx);
        drawOpaqueInstanced(ctx);
        drawSkybox(ctx);
      }

      framebuffer_->unbind();
      gd_.disableDepthTest();
    }

    void ForwardPass::uploadLights(const RenderPassContext& ctx) {
      if (!lightBuffer_ || ctx.lights.empty()) return;

      u32 count = static_cast<u32>(std::min(ctx.lights.size(), (size_t)kMaxGPULights));
      u32 sizeBytes = count * sizeof(GPULight);

      const bool isVulkan = dynamic_cast<VKDevice*>(&gd_) != nullptr;

      if (isVulkan) {
        auto* vkSSBO = dynamic_cast<VKStorageBuffer*>(lightBuffer_.get());
        if (!vkSSBO) return;

        vkSSBO->uploadData(ctx.lights.data(), static_cast<u16>(sizeBytes), 0);

        for (auto& cmd : ctx.queue->getRenderCommands()) {
          if (!cmd.material) continue;
          Pipeline* pipeline = cmd.material->getPipeline().get();
          if (!pipeline) continue;
          auto* vkPipeline = dynamic_cast<VKPipeline*>(pipeline);
          if (vkPipeline)
            vkSSBO->bindToPipeline(vkPipeline);
        }
      } else {
        lightBuffer_->bind();
        lightBuffer_->uploadData(ctx.lights.data(), static_cast<u16>(sizeBytes), 0);
      }
    }
    
    void ForwardPass::drawOpaque(const RenderPassContext& ctx)
    {
      
      const int lightCount = static_cast<int>(ctx.lights.size());
      static const char* kUseFlags[] = {
        "u_useAlbedoMap",
        "u_useNormalMap",
        "u_useRoughnessMap",
        "u_useMetallicMap"
      };

      static constexpr u32 kMaxTextureSlots = 4;
      const bool isVulkan = dynamic_cast<VKDevice*>(&gd_) != nullptr;

      for (auto& cmd : ctx.queue->getRenderCommands()) {
        if (!cmd.material || !cmd.mesh) continue;
        if (cmd.layer != RenderLayer::Opaque &&
          cmd.layer != RenderLayer::ForwardOnly) {
          continue;
        }

        for (u32 i = 0; i < kMaxTextureSlots; ++i) {
          cmd.material->setParameter(kUseFlags[i], 0);
        }

        if (isVulkan) {
          auto* vkPipeline = dynamic_cast<VKPipeline*>(cmd.material->getPipeline().get());
          if (!vkPipeline) continue;

          VKPipeline::TextureSetKey textureSet{};
          for (u32 slot = 0; slot < static_cast<u32>(cmd.textures.size()) &&
            slot < kMaxTextureSlots; ++slot) {
            if (!cmd.textures[slot]) continue;

            textureSet[slot] = cmd.textures[slot];
            cmd.material->setParameter(kUseFlags[slot], 1);
          }

          cmd.material->setParameter("u_view", ctx.view);
          cmd.material->setParameter("u_projection", ctx.projection);
          cmd.material->setParameter("u_model", cmd.transform);
          cmd.material->setParameter("u_viewPos", ctx.camera->position());
          cmd.material->setParameter("u_lightCount", lightCount);

          uploadProbeSHForCommand(ctx, cmd.material, cmd.transform);

          cmd.material->uploadParameters();

          VKMaterialDescriptor* descriptor = vkPipeline->getOrCreateMaterialDescriptorSet(textureSet);
          if (!descriptor) continue;
          descriptor->cpuData = vkPipeline->cpuUniformData();

          vkPipeline->uploadMaterialDescriptorUniforms(*descriptor);
          vkPipeline->bindForMaterial(descriptor->descriptorSet);

          gd_.draw(*cmd.mesh);
          continue;
        }

        cmd.material->bind();

        for (u32 slot = 0; slot < static_cast<u32>(cmd.textures.size()) &&
          slot < kMaxTextureSlots; ++slot) {
          if (!cmd.textures[slot]) continue;

          cmd.textures[slot]->bind(slot);

          const std::string samplerName = "u_texture" + std::to_string(slot);
          cmd.material->getPipeline()->setSamplerUniform(
            samplerName.c_str(), static_cast<int>(slot)
          );

          cmd.material->setParameter(kUseFlags[slot], 1);
        }

        cmd.material->setParameter("u_view", ctx.view);
        cmd.material->setParameter("u_projection", ctx.projection);
        cmd.material->setParameter("u_model", cmd.transform);
        cmd.material->setParameter("u_viewPos", ctx.camera->position());
        cmd.material->setParameter("u_lightCount", lightCount);

        uploadProbeSHForCommand(ctx, cmd.material, cmd.transform);

        cmd.material->uploadParameters();

        gd_.draw(*cmd.mesh);
      }
    }

    void ForwardPass::drawOpaqueInstanced(const RenderPassContext& ctx) {
        for (auto& cmd : ctx.queue->getInstancedCommands()) {
            if (!cmd.material || !cmd.mesh || !cmd.instanceBuffer) continue;
            if (cmd.instanceCount == 0) continue;

            cmd.material->bind();

            for (u32 slot = 0; slot < (u32)cmd.textures.size(); ++slot) {
                if (!cmd.textures[slot]) continue;
                cmd.textures[slot]->bind(slot);
                std::string samplerName = "u_texture" + std::to_string(slot);
                cmd.material->getPipeline()->setSamplerUniform(
                    samplerName.c_str(), (int)slot);
            }
            cmd.material->setParameter("u_view", ctx.view);
            cmd.material->setParameter("u_projection", ctx.projection);
            cmd.material->uploadParameters();
            gd_.drawInstanced(*cmd.mesh, cmd.instanceCount);
        }
    }

    void ForwardPass::drawSkybox(const RenderPassContext& ctx)
    {
      if (!skyboxMesh_ || !skyboxMat_ || !skyboxCubemap_) return;

      if (dynamic_cast<VKDevice*>(&gd_) != nullptr) return;

      glDepthFunc(GL_LEQUAL);
      gd_.disableFaceCulling();      

      skyboxMat_->bind();
      skyboxCubemap_->bind(0);
      skyboxMat_->getPipeline()->setSamplerUniform("u_texture0", 0);

      skyboxMat_->setParameter("u_view", ctx.view);
      skyboxMat_->setParameter("u_projection", ctx.projection);
      skyboxMat_->uploadParameters();

      gd_.draw(*skyboxMesh_);

      gd_.enableFaceCulling();
      glDepthFunc(GL_LESS);
    }

} // namespace mam