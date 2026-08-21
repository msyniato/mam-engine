#pragma once

#include "common/common.hpp"
#include "ecs/entity.hpp"
#include "ecs/engine_components.hpp"
#include "render/api/graphics_context.hpp"
#include "ui/terrain_gen_panel.hpp"

struct GLFWwindow;

namespace mam {

  class Context;
  class GraphicContext;

  /**
   * @brief ImGui panel that displays an interactive filesystem tree.
   *
   * Shows the assets directory as a two-pane browser: a directory tree on
   * the left and a content view on the right. Files can be clicked to
   * trigger drag-and-drop style assignment in other panels.
   */
  class FileExplorerPanel {
  public:
    /// Render the file explorer panel for the current ImGui frame.
    void render();

  private:
    void renderDirectoryTree(const std::filesystem::path& path);
    void renderContentBrowser();
    void handleFileClick(const std::filesystem::path& path);

    std::filesystem::path rootPath_    = "../assets"; ///< Root directory displayed by the browser.
    std::filesystem::path currentPath_ = "../assets"; ///< Currently open directory.
    std::string           selectedFile_ = "";          ///< Path of the last clicked file.
  };

  /**
   * @brief ImGui panel that displays and edits the components of the selected entity.
   *
   * The inspector shows component properties for the entity set via
   * setSelectedEntity(). Component values can be edited in-place.
   */
  class InspectorPanel {
  public:
    /**
     * @brief Set the entity whose components will be shown.
     * @param e Entity to inspect.
     */
    void setSelectedEntity(Entity e) { selectedEntity_ = e; }

    /// Returns the entity currently shown in the inspector.
    Entity getSelectedEntity() { return selectedEntity_; }

    /**
     * @brief Render the inspector panel for the given world.
     * @param world World that owns the selected entity.
     */
    void render(World* world);

  private:
    void renderTransformComponent(TransformComponent* tc);
    void renderRenderComponent(RenderComponent* rc, World* world);

    Entity      selectedEntity_     = kInvalidEntity; ///< Currently inspected entity.
    std::string droppedMeshPath_    = "None";          ///< Last mesh path dropped onto the panel.
    std::string droppedTexturePath_ = "None";          ///< Last texture path dropped onto the panel.
  };

  /**
   * @brief Abstract base class for the engine's ImGui integration.
   *
   * Manages initialisation, per-frame begin/end, and the standard set of
   * editor panels (world outliner, file explorer, inspector, terrain generator,
   * profiler, camera settings, viewport).
   *
   * Concrete subclasses (GLImGUIWrapper, VKImGUIWrapper) implement the
   * backend-specific begin/end frame and initialisation logic.
   */
  class ImGUIWrapper {
  public:
    ImGUIWrapper()          = default;
    virtual ~ImGUIWrapper() = default;

    /**
     * @brief Initialise the ImGui integration with the given window and graphics context.
     * @param window         Native GLFW window.
     * @param graphicsContext Active graphics context.
     */
    void init(GLFWwindow* window, GraphicsContext* graphicsContext);

    /// Begin a new ImGui frame. Must be called before any ImGui:: calls.
    virtual void beginFrame() = 0;

    /**
     * @brief Render all editor panels for the current frame.
     * @param context Application context providing access to ECS and subsystems.
     */
    void render(Context& context);

    /// Render a no-op dummy frame (used when no real rendering is needed).
    void renderDummyFrame();

    /// End the ImGui frame and submit draw data. Must be called after render().
    virtual void endFrame() = 0;

    /// Shut down the ImGui integration and release resources.
    void shutdown();

    /**
     * @brief Register a callback invoked by the terrain generation panel.
     * @param cb World-gen callback to forward to the terrain panel.
     */
    void setWorldGenCallback(WorldGenCallback cb) {
      terrainGenPanel_.setCallback(std::move(cb));
    }

    /**
     * @brief Permite establecer los parámetros iniciales del generador de mundo en el panel de terreno
     * @param params Parámetros del generador de mundo.
     */
    void setWorldGenParams(const mam::WorldGenParams& params) {
      terrainGenPanel_.setParams(params);
    }

    /// Returns the last known size of the scene viewport (pixels).
    glm::vec2 getViewportSize() { return lastViewportSize_; }

    /// Returns true when the camera is in free-look mode (not captured by ImGui).
    bool getIsCameraMovable() { return isCameraMovable_; }

    /// Callback type for injecting additional ImGui widgets each frame.
    using ExtraImGuiCallback = std::function<void(Context&)>;

    /**
     * @brief Register an extra callback rendered after the built-in panels.
     * @param cb Callback invoked each frame after all standard panels.
     */
    void setExtraImGuiCallback(ExtraImGuiCallback cb) {
      extraImGuiCb_ = std::move(cb);
    }

    /// Returns the top-left corner of the viewport region in screen coordinates.
    glm::vec2 getViewportMin() const { return lastViewportMin_; }

  protected:
    /**
     * @brief Backend-specific initialisation. Called from init().
     * @param window Native GLFW window handle.
     */
    virtual void initImplementation(GLFWwindow* window) = 0;

    /// Backend-specific shutdown. Called from shutdown().
    virtual void shutdownImplementation() = 0;

    FileExplorerPanel fileExplorer_;    ///< File explorer panel instance.
    InspectorPanel    inspector_;       ///< Component inspector panel instance.
    TerrainGenPanel   terrainGenPanel_; ///< Terrain/world generation panel instance.

    Entity       currentEntity_;        ///< Entity currently selected in the world outliner.
    glm::ivec2   lastViewportSize_;     ///< Last measured size of the scene viewport.
    glm::ivec2   lastViewportMin_;      ///< Last measured top-left of the scene viewport.
    ExtraImGuiCallback extraImGuiCb_;  ///< Optional extra widget callback.
    bool         isCameraMovable_;     ///< Whether the camera is currently controllable.

    GraphicsContext* graphicsContext_ = nullptr; ///< Active graphics context.

    /**
     * @brief Render the scene viewport widget. Overridable for backend-specific blitting.
     * @param context Application context.
     */
    virtual void renderViewport(Context& context);

  private:
    void initWrapper();
    void shutdownWrapper();
    void renderMainWindow();
    void renderWorldOutliner(Context& context);
    void renderFileExplorer();
    void renderTerrainGen();
    void renderInspector(Context& context);
    void renderProfiler(Context& context);
    void renderCameraSettings(Context& context);
  };

} // namespace mam
