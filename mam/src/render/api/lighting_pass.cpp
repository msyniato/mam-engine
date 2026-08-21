#include "render/api/lighting_pass.hpp"
#include "render/api/graphics_device.hpp"
#include "render/api/buffer.hpp"
#include "render/api/vertex_array.hpp"
#include "render/api/pipeline.hpp"
#include "render/api/shader.hpp"
#include "render/vulkan/vk_device.hpp"
#include "core/camera.hpp"

namespace mam {

  void LightingPass::init(u32 w, u32 h) {
    if (dynamic_cast<VKDevice*>(&gd_) != nullptr) return;
    FrameBufferSpec spec;
    spec.width        = w;
    spec.height       = h;
    spec.colorFormats = { Format::RGBA8 };
    spec.hasDepth     = false;
    output_ = gd_.createFrameBuffer(spec);

    auto vs = gd_.createShader();
    auto fs = gd_.createShader();
    vs->loadSourceFromFile(Shader::Type::Vertex,
        "../assets/shaders/deferred_light_vert.glsl");
    fs->loadSourceFromFile(Shader::Type::Fragment,
        "../assets/shaders/deferred_light_frag.glsl");
    vs->compile(); fs->compile();

    pipeline_ = gd_.createPipeline();
    pipeline_->addShader(vs.get());
    pipeline_->addShader(fs.get());
    auto err = pipeline_->create();
    assert(!err.has_value() && "Failed to link Lighting pipeline!");
    
    lightBuffer_ = gd_.createShaderStorageBuffer(0);

    initQuad();
  }

  void LightingPass::execute(const RenderPassContext& ctx) {
    if (!gBuffer_) return;

    output_->bind();

    u32 count = static_cast<u32>(std::min(ctx.lights.size(), (size_t)kMaxGPULights));
    u32 sizeBytes = count * sizeof(GPULight);
    lightBuffer_->bind();
    lightBuffer_->uploadData(ctx.lights.data(), static_cast<u16>(sizeBytes), 0);
    
    gd_.setClearColor(0.f, 0.f, 0.f, 1.f);
    gd_.clearBuffers();

    gd_.disableDepthTest();

    pipeline_->use();

    auto& colorAttachments = gBuffer_->getColorAttachments();

    if (colorAttachments.size() >= 4) {
      
      colorAttachments[0]->bind(0);
      pipeline_->setSamplerUniform("gPosition", 0);
      
      colorAttachments[1]->bind(1);
      pipeline_->setSamplerUniform("gNormal", 1);
      
      colorAttachments[2]->bind(2);
      pipeline_->setSamplerUniform("gAlbedoRoughness", 2);
      
      colorAttachments[3]->bind(3);
      pipeline_->setSamplerUniform("gMetallicAO", 3);
    }

    glm::vec3 camPos = ctx.camera->position();
    pipeline_->setUniform(
        "u_cameraPos",
        glm::value_ptr(camPos)
    );

    int lightCount = (int) ctx.lights.size();
    pipeline_->setUniform("lightCount", &lightCount);

    int ssaoEnabledInt = (ssaoEnabled && ssaoBuffer_) ? 1 : 0;
    pipeline_->setUniform("u_ssaoEnabled", &ssaoEnabledInt);

    if (ssaoEnabledInt && ssaoBuffer_) {
        auto& ssaoAttachments = ssaoBuffer_->getColorAttachments();
        if (!ssaoAttachments.empty()) {
            ssaoAttachments[0]->bind(4);
            pipeline_->setSamplerUniform("u_ssaoMap", 4);
        }
    }

    int shadowEnabledInt = 0;
    if (shadowBuffer_) {
        auto* depthTex = shadowBuffer_->getDepthAttachment().get();
        if (depthTex) {
            depthTex->bind(5);
            pipeline_->setSamplerUniform("u_shadowMap", 5);
            shadowEnabledInt = 1;
        }
    }
    pipeline_->setUniform("u_shadowEnabled", &shadowEnabledInt);

    if (shadowEnabledInt && !ctx.lightSpaceMatrices.empty()) {
        pipeline_->setUniform("u_lightSpace",
            glm::value_ptr(ctx.lightSpaceMatrices[0]));
    }

    drawQuad();

    output_->unbind();
  }

  void LightingPass::resize(u32 w, u32 h) {
    if (!output_) return;
    output_->resize(w, h);
  }

  void LightingPass::initQuad() {
    float verts[] = {
      -1.f,  1.f, 0.f, 1.f,
      -1.f, -1.f, 0.f, 0.f,
       1.f, -1.f, 1.f, 0.f,
      -1.f,  1.f, 0.f, 1.f,
       1.f, -1.f, 1.f, 0.f,
       1.f,  1.f, 1.f, 1.f,
    };
    quadVAO_ = gd_.createVertexArray();
    quadVAO_->bind();
    quadVBO_ = gd_.createVertexBuffer(nullptr, sizeof(verts));
    quadVBO_->uploadData(verts, sizeof(verts));
    quadVAO_->addVertexBuffer(
        quadVBO_.get(),
        0,
        2,
        UniformType::Float2,
        false,
        4 * sizeof(float),
        nullptr
    );

    quadVAO_->addVertexBuffer(
        quadVBO_.get(),
        1,
        2,
        UniformType::Float2,
        false,
        4 * sizeof(float),
        reinterpret_cast<const void*>(2 * sizeof(float))
    );
    quadVAO_->unBind();
  }

  void LightingPass::drawQuad() {
    quadVAO_->bind();
    gd_.draw(6u);
    quadVAO_->unBind();
  }
}