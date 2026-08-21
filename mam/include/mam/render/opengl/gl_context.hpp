#pragma once

#include "render/api/graphics_context.hpp"

namespace mam {

  class GLDevice;

  /**
   * @brief OpenGL implementation of the graphics context.
   *
   * Initialises GLAD, sets up the OpenGL state, and owns a GLDevice instance.
   */
  class GLContext : public GraphicsContext {
  public:
    GLContext();
    ~GLContext() override = default;

    /**
     * @brief Initialise the OpenGL context against an existing GLFW window.
     * @param window Platform window (must have a valid OpenGL context).
     */
    void init(Window* window) override;

    /// Begin a new frame (no-op for OpenGL).
    void beginFrame() override;

    /// End the current frame (no-op for OpenGL; the Window::swap() call handles presentation).
    void endFrame() override;

    /// Shut down and release the OpenGL device.
    void shutdown() override;

    /// Returns the underlying OpenGL graphics device.
    GraphicsDevice* getGraphicsDevice() override;

    /// Returns RenderAPI::OpenGL.
    RenderAPI getRenderAPI() const override { return RenderAPI::OpenGL; }

    /// No-op — OpenGL does not require explicit device idle waiting.
    void waitIdle() override {}

  private:
    std::unique_ptr<GLDevice> graphicsDevice_; ///< Owned OpenGL device.
  };

} // namespace mam
