#include "render/api/shadow_pass.hpp"
#include "render/api/graphics_device.hpp"
#include "render/api/renderer.hpp"
#include "render/api/mesh.hpp"
#include "render/api/pipeline.hpp"
#include "render/api/shader.hpp"
#include "matsys/material.hpp"
#include "core/camera.hpp"

#include "render/vulkan/vk_device.hpp"
#include "render/vulkan/vk_framebuffer.hpp"

namespace mam {

  void ShadowPass::init(u32, u32) {
    FrameBufferSpec spec;
    spec.width        = SHADOW_W;
    spec.height       = SHADOW_H;
    spec.colorFormats = {};          
    spec.hasDepth     = true;
    spec.depthFormat  = Format::DEPTH24STENCIL8;
    framebuffer_ = gd_.createFrameBuffer(spec);

    auto vs = gd_.createShader();
    auto fs = gd_.createShader();
    vs->loadSourceFromFile(Shader::Type::Vertex,
        "../assets/shaders/shadow_vert.glsl");
    fs->loadSourceFromFile(Shader::Type::Fragment,
        "../assets/shaders/shadow_frag.glsl");
    vs->compile(); fs->compile();

    pipeline_ = gd_.createPipeline();
    pipeline_->addShader(vs.get());
    pipeline_->addShader(fs.get());
    auto err = pipeline_->create();
    assert(!err.has_value() && "Failed to link Shadow pipeline!");
  }

  void ShadowPass::execute(const RenderPassContext& ctx) {
    if (ctx.lights.empty()) return;

    const GPULight* sunPtr = nullptr;
    for (const auto& l : ctx.lights) {
      if (l.type == LightType::DirectionalLight) { sunPtr = &l; break; }
    }
    if (!sunPtr) return;

    const GPULight& sun = *sunPtr;
    glm::vec3 lightDir  = glm::normalize(-sun.direction); 
    glm::vec3 lightPos  =  lightDir * 500.f;             

    glm::mat4 lightView = glm::lookAt(lightPos,
        glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
    glm::mat4 lightProj = glm::ortho(-orthoExtent_, orthoExtent_,
        -orthoExtent_, orthoExtent_, 1.f, 1500.f);
    lightSpaceMatrix_   = lightProj * lightView;

    framebuffer_->bind();
    gd_.clearBuffers();
    gd_.enableDepthTest();

    const bool isVulkan = dynamic_cast<VKDevice*>(&gd_) != nullptr;

    for (auto& cmd : ctx.queue->getRenderCommands()) {
      if (!cmd.mesh) continue;

      pipeline_->setUniform("u_model",
        glm::value_ptr(cmd.transform)
      );

      if (isVulkan) {pipeline_->setUniform(
          "u_projection",glm::value_ptr(lightSpaceMatrix_)
        );
      }else {
        pipeline_->setUniform(
          "u_lightSpace",glm::value_ptr(lightSpaceMatrix_)
        );
      }

      pipeline_->use();
      gd_.draw(*cmd.mesh);
    }

    framebuffer_->unbind();
  }

  void ShadowPass::resize(u32, u32) {
  }
}