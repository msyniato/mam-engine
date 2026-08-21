#pragma once

#include "common/common.hpp"
#include "render/api/graphics_context.hpp"
#include "render/api/probe_baker.hpp"
#include "ecs/light_probe.hpp"

namespace mam {

  class MaterialModuleRegistry;
  class MaterialRegistry;
  class GraphicsContext;
  class ImGUIWrapper;
  class InputManager;
  class WorldRegistry;
  class RenderQueue;
  class Renderer;
  class Window;
  class Context;
  class Camera;
  class JobSystem;

  /**
   * @brief Optional application layer that hooks into engine lifecycle events.
   *
   * Derive from Layer and pass an instance to Engine::AddLayer() to receive
   * per-frame callbacks without subclassing Engine itself.
   */
  class Layer {
  public:
    /// Called once after the engine has been fully initialised.
    void OnInit() {}

    /// Called every frame during the update phase.
    void OnUpdate() {}

    /// Called every frame during the render phase.
    void OnRender() {}

    /// Called every frame after the scene is rendered, for ImGui widgets.
    void OnImGuiRender() {}

    /// Called once before the engine shuts down.
    void OnCleanUp() {}
  };

  /**
   * @brief Top-level application engine that owns all engine subsystems.
   *
   * Engine creates and wires together the graphics context, window, renderer,
   * input manager, world registry, material system, job system, and ImGui
   * wrapper. Applications call init(), then update() each frame, and cleanup()
   * on exit. Optional Layer objects can be added to extend behaviour.
   */
  class Engine {
  public:
    /**
     * @brief Construct the engine from a graphics context descriptor.
     * @param desc Graphics context creation parameters (API, window size, title, etc.).
     */
    Engine(const GraphicsContext::Desc& desc);
    ~Engine();

    /// Initialise all subsystems. Must be called once before the first update().
    void init();

    /**
     * @brief Advance the engine by one tick.
     * @param context Application context providing service access to systems.
     */
    void update(Context& context);

    /// Shut down all subsystems and release resources.
    void cleanup();

    /**
     * @brief Register an application layer to receive lifecycle callbacks.
     * @param layer Shared pointer to the layer to add.
     */
    void AddLayer(std::shared_ptr<Layer> layer);

    /// Returns the material module registry (shader modules).
    MaterialModuleRegistry* getMaterialModuleRegistry() const;

    /// Returns the material registry (compiled materials).
    MaterialRegistry* getMaterialRegistry() const;

    /// Returns the world registry (ECS worlds).
    WorldRegistry* getWorldRegistry() const;

    /// Returns the active graphics context.
    GraphicsContext* getGraphicsContext() const;

    /// Returns the application window.
    Window* getWindow() const;

    /// Returns the ImGui wrapper.
    ImGUIWrapper* getImGUIWrapper() const;

    /// Returns the renderer.
    Renderer* getRenderer() const;

    /// Returns the input manager.
    InputManager* getInputManager() const;

    /// Returns the active camera.
    Camera* getCamera() const;

    /// Returns the job system.
    JobSystem* getJobSystem() const;

    /// Returns the active graphics API enumeration value.
    GraphicsContext::RenderAPI getRenderAPI() const;

  private:
    std::vector<std::shared_ptr<Layer>> layers; ///< Registered application layers.

    std::unique_ptr<MaterialModuleRegistry> materialModuleRegistry_; ///< Shader module registry.
    std::unique_ptr<MaterialRegistry>       materialRegistry_;        ///< Compiled material registry.
    std::unique_ptr<WorldRegistry>          worldRegistry_;           ///< ECS world registry.

    std::unique_ptr<ImGUIWrapper>    imguiWrapper_;      ///< ImGui integration.
    std::unique_ptr<GraphicsContext> graphicsContext_;   ///< Low-level graphics context.
    std::unique_ptr<InputManager>    inputManager_;      ///< Keyboard/mouse input.
    std::unique_ptr<RenderQueue>     renderQueue_;       ///< Frame render command queue.
    GraphicsContext::RenderAPI       api_;               ///< Active rendering API.
    std::unique_ptr<Renderer>        renderer_;          ///< High-level renderer.
    std::unique_ptr<Window>          window_;            ///< Application window.
    std::unique_ptr<Camera>          camera_;            ///< Active scene camera.
    std::unique_ptr<JobSystem>       jobSystem_;         ///< Work-stealing job system.

    ProbeBaker probeBaker_; ///< Light-probe baker.
    ProbeList  probeList_;  ///< List of light probes in the scene.
  };

} // namespace mam
