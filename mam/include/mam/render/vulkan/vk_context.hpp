
#pragma once

#include "render/api/graphics_context.hpp"
#include <vulkan/vulkan.hpp>

namespace mam{
  
  class VKDevice;
  
  /**
   * @brief Vulkan implementation of the graphics context.
   *
   * VKContext is the top-level owner of the Vulkan rendering backend.
   * It creates and manages the @ref VKDevice, ties it to a @ref Window,
   * and drives the per-frame begin/end lifecycle expected by the
   * engine's renderer.
   *
   * @note Copy and move operations are implicitly disabled through
   *       the unique_ptr member.
   */
  class VKContext : public GraphicsContext {
  public:

    /**
     * @brief Constructs a VKContext. Does not initialize Vulkan;
     *        call init() after construction.
     */
    VKContext();

    /**
     * @brief Destructor. Vulkan teardown should be triggered via shutdown()
     *        before the object is destroyed.
     */
    ~VKContext() override = default;

    /**
     * @brief Initializes the Vulkan context for the given window.
     *
     * Creates the VKDevice (instance, physical/logical device, swapchain,
     * render pass, command buffers, and synchronization primitives).
     *
     * @param window  Platform window that owns the Vulkan surface.
     *                Must remain valid for the lifetime of this context.
     */
    void init(Window* window) override;

    /**
     * @brief Begins a new render frame.
     *
     * Acquires the next swapchain image and opens the primary command buffer
     * for recording. Must be paired with a call to endFrame().
     */
    void beginFrame() override;

    /**
     * @brief Ends the current render frame.
     *
     * Submits the recorded command buffer to the graphics queue and
     * presents the finished image through the swapchain.
     */
    void endFrame() override;

    /**
     * @brief Shuts down the Vulkan context and releases all GPU resources.
     *
     * Waits for the device to become idle before destroying swapchain,
     * pipelines, buffers, and the logical device.
     */
    void shutdown() override;

    /**
     * @brief Returns the underlying graphics device.
     *
     * Provides access to the VKDevice for resource creation and
     * render-state configuration.
     *
     * @return Pointer to the GraphicsDevice (concrete type: VKDevice).
     */
    GraphicsDevice* getGraphicsDevice() override;

    /**
     * @brief Returns the render API identifier for this context.
     * @return RenderAPI::Vulkan.
     */
    RenderAPI getRenderAPI() const override { return RenderAPI::Vulkan; }

    /**
     * @brief Blocks the calling thread until the GPU has finished all work.
     *
     * Useful before destroying resources that may still be in use by the GPU.
     */
    void waitIdle() override;

  private:
    std::unique_ptr<VKDevice> graphicsDevice_; ///< Owned Vulkan device and swapchain.
    Window* window_;                           ///< Non-owning pointer to the platform window.
  };
  
} // namespace mam
