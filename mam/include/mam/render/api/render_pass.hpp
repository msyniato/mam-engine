#pragma once

#include "common/common.hpp"

namespace mam {

  class GraphicsDevice;
  class RenderQueue;
  class FrameBuffer;
  class Texture;
  class Camera;
  class ImGUIWrapper;
  class Context;

  /**
   * @brief Data bundle passed to each render pass during execution.
   *
   * Carries the graphics device, render queue, camera matrices, lights,
   * shared textures and framebuffers, and optional ImGui and app context pointers.
   */
  struct RenderPassContext {
    GraphicsDevice* gd;         ///< Active graphics device.
    RenderQueue*    queue;      ///< Render command queue for this frame.
    Camera*         camera;     ///< Active scene camera.
    glm::mat4       view;       ///< View matrix.
    glm::mat4       projection; ///< Projection matrix.

    std::vector<GPULight> lights; ///< Lights collected from the scene.

    std::unordered_map<std::string, Texture*>     textures;      ///< Named shared textures (e.g. shadow maps).
    std::unordered_map<std::string, FrameBuffer*> framebuffers;  ///< Named shared framebuffers.
    std::vector<glm::mat4>                        lightSpaceMatrices; ///< Light-space matrices for shadow rendering.

    ImGUIWrapper* imgui      = nullptr; ///< ImGui wrapper (Vulkan only; nullptr for OpenGL).
    Context*      appContext = nullptr; ///< Application ECS context (Vulkan only; nullptr for OpenGL).
  };

  /**
   * @brief Abstract base class for a single render pass in the render graph.
   *
   * Each pass owns its own output framebuffer and is responsible for
   * initialisation, per-frame execution, and viewport resize handling.
   */
  class RenderPass {
  public:
    virtual ~RenderPass() = default;

    /**
     * @brief Initialise pass resources at the given viewport size.
     * @param width  Viewport width in pixels.
     * @param height Viewport height in pixels.
     */
    virtual void init(u32 width, u32 height) = 0;

    /**
     * @brief Execute this pass for the current frame.
     * @param ctx Frame context containing scene data and shared resources.
     */
    virtual void execute(const RenderPassContext& ctx) = 0;

    /**
     * @brief Handle a viewport resize.
     * @param width  New viewport width in pixels.
     * @param height New viewport height in pixels.
     */
    virtual void resize(u32 width, u32 height) = 0;

    /**
     * @brief Returns the output framebuffer produced by this pass (may be nullptr).
     */
    virtual FrameBuffer* getOutput() const { return nullptr; }

    bool        enabled = true; ///< When false the pass is skipped during graph execution.
    std::string name;           ///< Unique name used to look up the pass in the render graph.
  };

} // namespace mam
