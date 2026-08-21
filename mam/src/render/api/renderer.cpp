#include "render/api/renderer.hpp"

#include "core/camera.hpp"
#include "ecs/world.hpp"
#include "matsys/material.hpp"
#include "render/api/buffer.hpp"
#include "render/api/graphics_device.hpp"
#include "render/api/shader.hpp"
#include "render/api/vertex_array.hpp"
#include "render/api/pipeline.hpp"
#include "render/api/mesh.hpp"
#include "render/api/forward_pass.hpp"
#include "render/api/shadow_pass.hpp"
#include "render/api/geometry_pass.hpp"
#include "render/api/lighting_pass.hpp"
#include "render/api/ssao_pass.hpp"

namespace mam{
  
  Renderer::Renderer(GraphicsDevice& gd, const glm::vec2& screen_size) :
    graphicsDevice_(gd)
  {
    u32 w = static_cast<u32>(screen_size.x);
    u32 h = static_cast<u32>(screen_size.y);

    if (!isDeferred_) {
      shadowPass_ = renderGraph_.addPass<ShadowPass>("shadow", gd);
      forwardPass_ = renderGraph_.addPass<ForwardPass>("forward", gd);
    }
    else {
      shadowPass_ = renderGraph_.addPass<ShadowPass>("shadow", gd);  
      geometryPass_ = renderGraph_.addPass<GeometryPass>("geometry", gd);
      ssaoPass_ = renderGraph_.addPass<SSAOPass>("ssao", gd);
      ssaoPass_->enabled = false;
      lightingPass_ = renderGraph_.addPass<LightingPass>("lighting", gd);
      forwardPass_ = renderGraph_.addPass<ForwardPass>("forward", gd);
    }

    renderGraph_.init(w, h);

    if (geometryPass_ && ssaoPass_)
      ssaoPass_->setGBuffer(geometryPass_->getOutput());

    if (geometryPass_ && lightingPass_)
      lightingPass_->setGBuffer(geometryPass_->getOutput());

    if (ssaoPass_ && lightingPass_)
      lightingPass_->setSSAOMap(ssaoPass_->getOutput());

    if (shadowPass_ && lightingPass_)
      lightingPass_->setShadowMap(shadowPass_->getOutput());

    if (lightingPass_ && forwardPass_)
      forwardPass_->setLightingOutput(lightingPass_->getOutput());

    if (geometryPass_ && forwardPass_)
      forwardPass_->setGBuffer(geometryPass_->getOutput());
  }

  Renderer::~Renderer() {
   
  }

  void Renderer::initQuad() {
    
    float verts[] = {
      -1.0f,  1.0f,   0.0f, 1.0f,
      -1.0f, -1.0f,   0.0f, 0.0f,
       1.0f, -1.0f,   1.0f, 0.0f,

      -1.0f,  1.0f,   0.0f, 1.0f,
       1.0f, -1.0f,   1.0f, 0.0f,
       1.0f,  1.0f,   1.0f, 1.0f,
    };
    
    quadVAO_ = graphicsDevice_.createVertexArray();
    assert(quadVAO_ && "Failed to create screen quad VAO!");
    quadVAO_->bind();
    
    // TODO: create index buffer
    
    quadVBO_ = graphicsDevice_.createVertexBuffer(nullptr, sizeof(verts));
    quadVBO_->uploadData(verts, sizeof(verts));
    assert(quadVBO_ && "Failed to create screen quad VBO!");
    
    quadVAO_->addVertexBuffer(quadVBO_.get(), 0, 2, Float1, false, 4 * sizeof(float), nullptr);
    quadVAO_->addVertexBuffer(quadVBO_.get(), 1, 2, Float1, false, 4 * sizeof(float), reinterpret_cast<const void*>(2 * sizeof(float)));
    
    quadVAO_->unBind();
  }

  void Renderer::drawQuad() {
    quadVAO_->bind();
    graphicsDevice_.draw(6);
    quadVAO_->unBind();
  }
  
  void Renderer::render(RenderQueue& renderQueue, Camera& camera) {
    render(renderQueue, camera, nullptr, nullptr);
  }

  void Renderer::render(RenderQueue& renderQueue,
    Camera& camera,
    ImGUIWrapper* imgui,
    Context* appContext) {

    glm::mat4 view = glm::lookAt(
      camera.position(),
      camera.position() + camera.front(),
      camera.up()
    );

    glm::mat4 projection = glm::perspective(
      glm::radians(camera.fov()),
      camera.aspectRatio(),
      camera.zNear(),
      camera.zFar()
    );

    RenderPassContext ctx;
    ctx.queue = &renderQueue;
    ctx.camera = &camera;
    ctx.gd = &graphicsDevice_;
    ctx.view = view;
    ctx.projection = projection;
    ctx.lights = lightList_.lights;

    ctx.imgui = imgui;
    ctx.appContext = appContext;
   
    const GPULight* sunPtr = nullptr;
    for (const auto& l : ctx.lights)
        if (l.type == LightType::DirectionalLight) { sunPtr = &l; break; }

    if (shadowPass_ && sunPtr) {
        const GPULight& sun     = *sunPtr;
        glm::vec3 lightDir      = glm::normalize(-sun.direction);
        glm::vec3 lightPos      =  lightDir * 500.f;
        glm::mat4 lightView     = glm::lookAt(lightPos,
                                              glm::vec3(0.f),
                                              glm::vec3(0.f, 1.f, 0.f));
        const float e           = shadowPass_->getOrthoExtent();
        glm::mat4 lightProj     = glm::ortho(-e, e, -e, e, 1.f, 1500.f);
        ctx.lightSpaceMatrices.push_back(lightProj * lightView);
    }

    renderGraph_.execute(ctx);
    renderQueue.clear();

  }

  void Renderer::resize(u32 width, u32 height) {
    renderGraph_.resize(width, height);
  }

  void Renderer::setSkybox(Texture* cubemap, Material* mat)
  {
    if (!cubemap || !mat || !forwardPass_) return;

    const glm::vec3 p[8] = {
      {-1,-1,-1}, { 1,-1,-1}, { 1, 1,-1}, {-1, 1,-1},
      {-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1}
    };
    std::vector<Vertex> verts;
    verts.reserve(8);
    for (const auto& pos : p) {
      Vertex v{};
      v.position = pos;
      verts.push_back(v);
    }
    std::vector<unsigned int> idx = {
      0,1,2, 2,3,0,   4,5,6, 6,7,4,   0,4,7, 7,3,0,
      1,5,6, 6,2,1,   3,7,6, 6,2,3,   0,4,5, 5,1,0
    };

    skyboxCube_ = std::make_unique<Mesh>(graphicsDevice_, "skybox_cube", verts, idx);
    forwardPass_->setSkybox(skyboxCube_.get(), mat, cubemap);
  }

  void Renderer::setSSAOEnabled(bool on)
  {
    ssaoEnabled_ = on;
    if (ssaoPass_)     ssaoPass_->enabled = on && isDeferred_;
    if (lightingPass_) lightingPass_->ssaoEnabled = on;
  }

  float Renderer::getSSAOPassMs() const
  {
    if (ssaoPass_ && ssaoPass_->enabled)
        return ssaoPass_->getLastPassMs();
    return 0.f;
  }

  void Renderer::setShadowOrthoExtent(float halfExtent)
  {
    if (shadowPass_) shadowPass_->setOrthoExtent(halfExtent);
  }

  void Renderer::setDeferredEnabled(bool on)
  {
    isDeferred_ = on;

    if (geometryPass_) geometryPass_->enabled = on;
    if (ssaoPass_)     ssaoPass_->enabled = on && ssaoEnabled_;
    if (lightingPass_) lightingPass_->enabled = on;
    if (shadowPass_)   shadowPass_->enabled = !on;
    if (forwardPass_)  forwardPass_->enabled = true;
  }

  FrameBuffer* Renderer::getFramebuffer() const {
    if (forwardPass_)  return forwardPass_->getOutput();
    //if (lightingPass_) return lightingPass_->getOutput();
    return nullptr;
}

  // Render queue
  void RenderQueue::submit(const RenderCommand& cmd) {
    renderCommands_.push_back(cmd);
  }
  
  void RenderQueue::submitInstanced(const RenderCommandInstanced& cmd) {
    instancedCommands_.push_back(cmd);
  }
  
  std::vector<RenderCommand>& RenderQueue::getRenderCommands() {
    return renderCommands_;
  }
  
  std::vector<RenderCommandInstanced>& RenderQueue::getInstancedCommands() {
    return instancedCommands_;
  }
  
  void RenderQueue::clear() {
    renderCommands_.clear();
    instancedCommands_.clear();
  }
  
  // Static batcher
  void StaticBatcher::add(Mesh* mesh, const glm::mat4& transform) {
    entries_.push_back({ mesh, transform });
    built_ = false;
  }

  void StaticBatcher::build(GraphicsDevice& gd, const std::string& name) {
    std::vector<Vertex>       verts;
    std::vector<unsigned int> indices;
    unsigned int baseVertex = 0;

    for (auto& e : entries_) {
      for (auto v : e.mesh->vertices()) {
        glm::vec4 p = e.transform * glm::vec4(v.position, 1.f);
        v.position = glm::vec3(p);
        glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(e.transform)));
        v.normal = glm::normalize(normalMat * v.normal);
        verts.push_back(v);
      }
      u32 idxCount = e.mesh->indices();
      auto* va = e.mesh->vertexArray().get();
      for (u32 i = 0; i < idxCount; ++i)
        indices.push_back(baseVertex + i);
      baseVertex += static_cast<u32>(e.mesh->vertices().size());
    }

    batchedMesh_ = std::make_unique<Mesh>(gd, name, verts, indices);
    built_ = true;
  }

  void StaticBatcher::clear() {
    entries_.clear();
    batchedMesh_.reset();
    built_ = false;
  }
  
} // namespace mam
