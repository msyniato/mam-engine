#pragma once

#include "render/api/window.hpp"

struct GLFWwindow;

namespace mam {

  /**
   * @brief OpenGL window backed by GLFW.
   *
   * Creates and manages a GLFW window with an OpenGL context,
   * handles buffer swapping, and forwards events.
   */
  class GLWindow : public Window
  {
  public:
    /**
     * @brief Create a GLFW window with the given dimensions.
     * @param w Window width in pixels.
     * @param h Window height in pixels.
     */
    GLWindow(const u16 w, const u16 h);
    ~GLWindow() override;

    void render()       override;
    bool isClosed()     override;
    void cleanup()      override;
    void swap()         override;
    void processEvents() override;

    /// Returns the underlying GLFW window handle.
    GLFWwindow* getNativeWindow() override { return glfw_window_; }

    /// Returns the window height in pixels.
    u16 getHeight() override;

    /// Returns the window width in pixels.
    u16 getWidth()  override;

  private:
    GLFWwindow* glfw_window_; ///< Native GLFW window handle.
  };

} // namespace mam
