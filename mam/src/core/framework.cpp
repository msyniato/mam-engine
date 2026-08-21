#include "core/framework.hpp"
#include "core/camera.hpp"
#include "core/input.hpp"

#include "ecs/world.hpp"
#include "matsys/material_module.hpp"
#include "matsys/material.hpp"
#include "matsys/shader_composer.hpp"

#include "render/api/renderer.hpp"
#include "render/api/graphics_device.hpp"

#include "render/vulkan/vk_device.hpp"

#include "render/api/window.hpp"
#include "render/api/frame_buffer.hpp"
#include "render/opengl/gl_window.hpp"
#include "render/vulkan/vk_window.hpp"

#include "render/opengl/gl_context.hpp"
#include "render/vulkan/vk_context.hpp"
#include "render/vulkan/vk_framebuffer.hpp"

#include "jobsys/job_system.hpp"
#include "jobsys/dispatcher.hpp"

#include "ui/imgui_wrapper.hpp"
#include "ui/gl_imgui_wrapper.hpp"
#include "ui/vk_imgui_wrapper.hpp"

namespace mam
{
  Engine::Engine(const GraphicsContext::Desc& desc) {
    api_ = desc.api;

    switch (api_) {
    case GraphicsContext::RenderAPI::OpenGL:
      window_ = std::make_unique<GLWindow>(desc.width, desc.height);
      graphicsContext_ = std::make_unique<GLContext>();
      imguiWrapper_ = std::make_unique<GLImGUIWrapper>();
      break;

    case GraphicsContext::RenderAPI::Vulkan:
      window_ = std::make_unique<VKWindow>(desc.width, desc.height);
      graphicsContext_ = std::make_unique<VKContext>();
      imguiWrapper_ = std::make_unique<VKImGUIWrapper>();
      break;

    case GraphicsContext::RenderAPI::Invalid:
    default:
      assert(false && "No graphics API is set!");
      break;
    }

    assert(window_ && "Failed to create engine window!");
    assert(graphicsContext_ && "Failed to create graphics context!");
    assert(imguiWrapper_ && "Failed to create ImGUI manager!");

    glm::vec2 size = { window_->getWidth(), window_->getHeight() };

    materialModuleRegistry_ = std::make_unique<MaterialModuleRegistry>();
    assert(materialModuleRegistry_ && "Failed to create material module registry!");

    worldRegistry_ = std::make_unique<WorldRegistry>();
    assert(worldRegistry_ && "Failed to create world registry!");

    renderQueue_ = std::make_unique<RenderQueue>();
    assert(renderQueue_ && "Failed to create render queue!");

    inputManager_ = std::make_unique<InputManager>(window_->getNativeWindow());
    assert(inputManager_ && "Failed to create input manager!");

    camera_ = std::make_unique<Camera>(*inputManager_, size);
    assert(camera_ && "Failed to create camera!");

    jobSystem_ = std::make_unique<JobSystem>();
    assert(jobSystem_ && "Failed to create job system!");
    jobSystem_->Startup();

    if (api_ == GraphicsContext::RenderAPI::OpenGL) {
      auto graphicsDevice = graphicsContext_->getGraphicsDevice();
      assert(graphicsDevice && "OpenGL graphics device is null!");

      materialRegistry_ =
        std::make_unique<MaterialRegistry>(*materialModuleRegistry_, *graphicsDevice);
      assert(materialRegistry_ && "Failed to create material registry!");

      renderer_ = std::make_unique<Renderer>(*graphicsDevice, size);
      assert(renderer_ && "Failed to create renderer!");
    }
  }

  Engine::~Engine() {
  }

  MaterialModuleRegistry* Engine::getMaterialModuleRegistry() const {
    return materialModuleRegistry_.get();
  }

  MaterialRegistry* Engine::getMaterialRegistry() const {
    return materialRegistry_.get();
  }

  WorldRegistry* Engine::getWorldRegistry() const {
    return worldRegistry_.get();
  }

  GraphicsContext* Engine::getGraphicsContext() const {
    return graphicsContext_.get();
  }

  Window* Engine::getWindow() const {
    return window_.get();
  }

  Renderer* Engine::getRenderer() const {
    return renderer_.get();
  }

  InputManager* Engine::getInputManager() const {
    return inputManager_.get();
  }

  ImGUIWrapper* Engine::getImGUIWrapper() const {
    return imguiWrapper_.get();
  }

  Camera* Engine::getCamera() const {
    return camera_.get();
  }

  JobSystem* Engine::getJobSystem() const {
    return jobSystem_.get();
  }

  GraphicsContext::RenderAPI Engine::getRenderAPI() const {
    return api_;
  }

  void Engine::AddLayer(std::shared_ptr<Layer> layer)
  {
  }

  void Engine::init()
  {
    graphicsContext_->init(getWindow());

    auto graphicsDevice = graphicsContext_->getGraphicsDevice();
    assert(graphicsDevice && "Graphics device is null after graphics context init!");

    glm::vec2 size = { window_->getWidth(), window_->getHeight() };

    if (api_ == GraphicsContext::RenderAPI::Vulkan) {
      materialRegistry_ =
        std::make_unique<MaterialRegistry>(*materialModuleRegistry_, *graphicsDevice);
      assert(materialRegistry_ && "Failed to create material registry!");

      renderer_ = std::make_unique<Renderer>(*graphicsDevice, size);
      assert(renderer_ && "Failed to create renderer!");

      if (auto* vkdev = dynamic_cast<VKDevice*>(graphicsDevice)) {
        if (auto* vkfb = dynamic_cast<VKFramebuffer*>(renderer_->getFramebuffer())) {
          vkdev->setCurrentRenderPass(vkfb->renderPass());
          vkdev->setCurrentColorAttachmentCount(
            static_cast<u32>(vkfb->spec().colorFormats.size())
          );
        }
      }
    }

    inputManager_->init();
    imguiWrapper_->init(window_->getNativeWindow(), graphicsContext_.get());

    if (api_ == GraphicsContext::RenderAPI::Vulkan)
    {
      materialRegistry_->setShaderTemplatePaths(
        "../assets/shaders/templates/vulkan/material_vk.vert",
        "../assets/shaders/templates/vulkan/material_vk.frag"
      );

      ID basicTransformVk = materialModuleRegistry_->registerModule(
        "../assets/shaders/modules/vulkan/basic_transform_vk.module.glsl");

      ID basicTextureVk = materialModuleRegistry_->registerModule(
        "../assets/shaders/modules/vulkan/basic_texture_vk.module.glsl");

      ID basicLitTextureVk = materialModuleRegistry_->registerModule(
        "../assets/shaders/modules/vulkan/basic_lit_texture_vk.module.glsl");

      ID basicColorVk = materialModuleRegistry_->registerModule(
        "../assets/shaders/modules/vulkan/basic_color_vk.module.glsl");

      ID normalDebugVk = materialModuleRegistry_->registerModule(
        "../assets/shaders/modules/vulkan/normal_debug_vk.module.glsl");

      ID pbrTransformVk = materialModuleRegistry_->registerModule(
        "../assets/shaders/modules/vulkan/pbr_transform_vk.module.glsl");

      ID disneyBsdfVk = materialModuleRegistry_->registerModule(
        "../assets/shaders/modules/vulkan/disney_bsdf_vk.module.glsl");

      ID pbrLightingVk = materialModuleRegistry_->registerModule(
        "../assets/shaders/modules/vulkan/pbr_lighting_vk.module.glsl");

      ID pbrSurfaceVk = materialModuleRegistry_->registerModule(
        "../assets/shaders/modules/vulkan/pbr_surface_vk.module.glsl");

      if (basicTransformVk != kInvalidID && basicTextureVk != kInvalidID)
        materialRegistry_->createMaterial("vk_texture_mat",
          { basicTransformVk, basicTextureVk });

      if (basicTransformVk != kInvalidID && basicLitTextureVk != kInvalidID)
        materialRegistry_->createMaterial("vk_lit_texture_mat",
          { basicTransformVk, basicLitTextureVk });

      if (basicTransformVk != kInvalidID && basicColorVk != kInvalidID)
        materialRegistry_->createMaterial("vk_color_mat",
          { basicTransformVk, basicColorVk });

      if (basicTransformVk != kInvalidID && normalDebugVk != kInvalidID)
        materialRegistry_->createMaterial("vk_normal_debug_mat",
          { basicTransformVk, normalDebugVk });

      if (pbrTransformVk != kInvalidID &&
        disneyBsdfVk != kInvalidID &&
        pbrLightingVk != kInvalidID &&
        pbrSurfaceVk != kInvalidID)
      {
        materialRegistry_->createMaterial("pbr_mat",
          { pbrTransformVk, disneyBsdfVk, pbrLightingVk, pbrSurfaceVk });
      }else{
        if (basicTransformVk != kInvalidID && basicLitTextureVk != kInvalidID)
          materialRegistry_->createMaterial("pbr_mat",
            { basicTransformVk, basicLitTextureVk });
      }
    } else {
      std::vector<ID> modules;
      std::vector<ID> terrainModules;
      std::vector<ID> pbrModules;
      std::vector<ID> vegModules;
      std::vector<ID> skyboxModules;
      
      modules.push_back(
        materialModuleRegistry_->registerModule("../assets/shaders/modules/fragment/basic_texture.module.glsl")
      );

      terrainModules.push_back(
        materialModuleRegistry_->registerModule("../assets/shaders/modules/fragment/terrain_splatting.module.glsl")
      );

      vegModules.push_back(materialModuleRegistry_->registerModule(
        "../assets/shaders/modules/fragment/vegetation_frag.module.glsl"));

      skyboxModules.push_back(materialModuleRegistry_->registerModule(
				"../assets/shaders/modules/fragment/skybox_frag.module.glsl"));

      modules.push_back(
        materialModuleRegistry_->registerModule("../assets/shaders/modules/vertex/uv_pass.module.glsl")
      );

      modules.push_back(
        materialModuleRegistry_->registerModule("../assets/shaders/modules/vertex/basic_transform.module.glsl")
      );

      terrainModules.push_back(
        materialModuleRegistry_->registerModule("../assets/shaders/modules/vertex/terrain_splatting_ver.module.glsl")
      );

      terrainModules.push_back(
        materialModuleRegistry_->getModule("uv_pass")->getID()
      );

      terrainModules.push_back(
        materialModuleRegistry_->getModule("basic_transform")->getID()
      );

      vegModules.push_back(materialModuleRegistry_->registerModule(
        "../assets/shaders/modules/vertex/vegetation_transform_vert.module.glsl"));

			skyboxModules.push_back(materialModuleRegistry_->registerModule("../assets/shaders/modules/vertex/skybox_vert.module.glsl"));

      materialModuleRegistry_->registerModule(
        "../assets/shaders/modules/fragment/disney_bsdf.module.glsl");

      materialModuleRegistry_->registerModule(
        "../assets/shaders/modules/fragment/pbr_lighting.module.glsl");

      materialModuleRegistry_->registerModule(
        "../assets/shaders/modules/fragment/pbr_surface.module.glsl");

      
      pbrModules.push_back(materialModuleRegistry_->getModule("disney_bsdf")->getID());
      pbrModules.push_back(materialModuleRegistry_->getModule("pbr_lighting")->getID());
      pbrModules.push_back(materialModuleRegistry_->getModule("pbr_surface")->getID());
      pbrModules.push_back(materialModuleRegistry_->getModule("uv_pass")->getID());
      pbrModules.push_back(materialModuleRegistry_->getModule("basic_transform")->getID());

      materialRegistry_->setShaderTemplatePaths(
        "../assets/shaders/templates/opengl/vertex.template.glsl",
        "../assets/shaders/templates/opengl/fragment.template.glsl"
      );
      
      materialRegistry_->createMaterial("basic_mat", modules);
      materialRegistry_->createMaterial("terrain_mat", terrainModules);
      materialRegistry_->createMaterial("pbr_mat", pbrModules);
      materialRegistry_->createMaterial("vegetation_mat", vegModules);
      materialRegistry_->createMaterial("skybox_mat", skyboxModules);

    }

    auto world = worldRegistry_->createWorld();
    world->createScene();
  }

  void Engine::update(Context& context)
  {
    double dt = 0.0f;

    context.Add<World>(worldRegistry_->getCurrentWorld());
    context.Add<RenderQueue>(renderQueue_.get());
    context.Add<Camera>(camera_.get());
    context.Add<FrameBuffer>(renderer_->getFramebuffer());
    context.Add<InputManager>(inputManager_.get());
    context.Add<JobSystem>(jobSystem_.get());
    context.Add<LightList>(&renderer_->getLightsList());
    context.Add<ProbeBaker>(&probeBaker_);
    context.Add<ProbeList>(&probeList_);

    glfwSwapInterval(1);

    uint64_t frame_count = 0;

    glm::ivec2 pendingViewportSize = { 0, 0 };

    while (!window_->isClosed())
    {
      auto start_time = glfwGetTime();

      using Clock = std::chrono::high_resolution_clock;
      using Ms = std::chrono::duration<double, std::milli>;
      auto t0 = Clock::now();

      DrainMainQueue();

      if (api_ == GraphicsContext::RenderAPI::Vulkan)
      {
        window_->processEvents();

        if (pendingViewportSize.x > 0 && pendingViewportSize.y > 0) {
          auto* fb = renderer_->getFramebuffer();
          u32 curW = fb ? fb->spec().width : 0;
          u32 curH = fb ? fb->spec().height : 0;

          u32 newW = static_cast<u32>(pendingViewportSize.x);
          u32 newH = static_cast<u32>(pendingViewportSize.y);

          if (newW != curW || newH != curH) {
            auto* vkContext = dynamic_cast<VKContext*>(graphicsContext_.get());
            if (vkContext) vkContext->waitIdle();
            renderer_->resize(newW, newH);
            if (vkContext) vkContext->waitIdle();
          }
          pendingViewportSize = { 0, 0 };
        }

        graphicsContext_->beginFrame();
        imguiWrapper_->beginFrame();

        camera_->update(dt,
          imguiWrapper_->getViewportSize(),
          imguiWrapper_->getIsCameraMovable());

        glm::ivec2 vp = imguiWrapper_->getViewportSize();
        if (vp.x > 0 && vp.y > 0)
          pendingViewportSize = vp;

        worldRegistry_->getCurrentWorld()->updateSystems(context);

        if (context.Get<ProbeList>())
          renderer_->render(*renderQueue_, *camera_, nullptr, &context);
        else
          renderer_->render(*renderQueue_, *camera_);
        if (jobSystem_) jobSystem_->wait_idle();

        context.Add<FrameBuffer>(renderer_->getFramebuffer());

        imguiWrapper_->render(context);
        imguiWrapper_->endFrame();

        graphicsContext_->endFrame();
        window_->swap();

        auto end_time = glfwGetTime();
        dt = end_time - start_time;
        worldRegistry_->getCurrentWorld()->lastDeltaTime = dt;
      }
      else
      {
        camera_->update(
          dt,
          imguiWrapper_->getViewportSize(),
          imguiWrapper_->getIsCameraMovable()
        );
        auto t1 = Clock::now();

        glm::ivec2 vp = imguiWrapper_->getViewportSize();
        if (vp.x > 0 && vp.y > 0)
          renderer_->resize(static_cast<u32>(vp.x), static_cast<u32>(vp.y));

        worldRegistry_->getCurrentWorld()->updateSystems(context);

        window_->processEvents();
        graphicsContext_->beginFrame();

        if (context.Get<ProbeList>())
          renderer_->render(*renderQueue_, *camera_, nullptr, &context);
        else
          renderer_->render(*renderQueue_, *camera_);
        if (jobSystem_) jobSystem_->wait_idle();

        graphicsContext_->endFrame();

        imguiWrapper_->beginFrame();
        imguiWrapper_->render(context);
        imguiWrapper_->endFrame();

        window_->swap();

        auto end_time = glfwGetTime();
        dt = end_time - start_time;
        worldRegistry_->getCurrentWorld()->lastDeltaTime = dt;
      }
    }
  }

  void Engine::cleanup()
  {
    if (api_ == GraphicsContext::RenderAPI::Vulkan){
      auto* vkContext = dynamic_cast<VKContext*>(graphicsContext_.get());

      if (vkContext) vkContext->waitIdle();
      if (imguiWrapper_) imguiWrapper_->shutdown();
      
      if (vkContext) vkContext->waitIdle();
      materialRegistry_.reset();

      if (vkContext) vkContext->waitIdle();
      worldRegistry_.reset();

      if (vkContext) vkContext->waitIdle();
      renderer_.reset();

      if (vkContext) vkContext->waitIdle();
      if (graphicsContext_) graphicsContext_->shutdown();
      
    } else {
      if (imguiWrapper_) imguiWrapper_->shutdown();
      if (graphicsContext_) graphicsContext_->shutdown();
    }

    if (jobSystem_) jobSystem_->Shutdown();
    
  }

} // namespace mam