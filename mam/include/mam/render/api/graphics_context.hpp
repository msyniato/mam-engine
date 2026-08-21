#pragma once

#include "common/common.hpp"

namespace mam {

  class GraphicsDevice;
  class Window;

  /**
   * @brief Abstract graphics context.
   *
   * Manages the lifetime of the graphics API connection (OpenGL or Vulkan),
   * drives per-frame begin/end calls, and exposes the underlying GraphicsDevice.
   */
  class GraphicsContext
  {
  public:
    virtual ~GraphicsContext() = 0;

    /**
     * @brief Available rendering back-ends.
     */
    enum class RenderAPI
    {
      Invalid = 0, ///< Uninitialized / invalid.
      OpenGL  = 1, ///< OpenGL 4.5+ back-end.
      Vulkan       ///< Vulkan back-end.
    };

    /**
     * @brief Descriptor used to create a GraphicsContext.
     */
    struct Desc {
      u32       width;  ///< Initial viewport width in pixels.
      u32       height; ///< Initial viewport height in pixels.
      RenderAPI api;    ///< Requested rendering back-end.
    };

    /**
     * @brief Initialise the context against an existing window.
     * @param window Platform window to attach to.
     */
    virtual void init(Window* window) = 0;

    /// Begin a new frame (API-specific synchronisation / acquire).
    virtual void beginFrame() = 0;

    /// End the current frame (submit, present, etc.).
    virtual void endFrame() = 0;

    /// Shut down and release all context resources.
    virtual void shutdown() = 0;

    /// Returns the underlying graphics device.
    virtual GraphicsDevice* getGraphicsDevice() = 0;

    /// Returns the active rendering back-end.
    virtual RenderAPI getRenderAPI() const = 0;

    /// Block until the device has finished all in-flight work (no-op for OpenGL).
    virtual void waitIdle() {}
  };

} // namespace mam
