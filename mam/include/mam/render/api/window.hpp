#pragma once

#include "common/common.hpp"
#include <functional>

namespace mam {

  /**
   * @brief Abstract window interface.
   *
   * Wraps the platform-specific window (currently GLFW) and exposes
   * rendering, event processing, and dimension queries.
   */
  class Window
  {
  public:
    Window() {}
    virtual ~Window() = default;

    /// Render any window-level decorations or overlays.
    virtual void render() = 0;

    /// Returns true if the window has been closed by the user.
    virtual bool isClosed() = 0;

    /// Release all window resources.
    virtual void cleanup() = 0;

    /// Returns the underlying native GLFW window handle.
    virtual GLFWwindow* getNativeWindow() = 0;

    /// Swap the front and back buffers (double-buffered rendering).
    virtual void swap() = 0;

    /// Poll and dispatch platform events (keyboard, mouse, resize, etc.).
    virtual void processEvents() = 0;

    /// Returns the window height in pixels.
    virtual u16 getHeight() = 0;

    /// Returns the window width in pixels.
    virtual u16 getWidth() = 0;

  protected:
    u16 width_;  ///< Window width in pixels.
    u16 height_; ///< Window height in pixels.
  };

} // namespace mam
