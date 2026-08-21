
#pragma once

#include "render/api/window.hpp"

#include <vulkan/vulkan.hpp>
#include <optional>

struct GLFWwindow;
struct VKDevice;

namespace mam {

	/**
	* @brief Vulkan-backed window implementation using GLFW.
	*
	* Creates and manages a native OS window through GLFW and exposes
	* the Vulkan-compatible surface needed for presentation. Also tracks
	* framebuffer resize events so the swapchain can be recreated at the
	* right moment.
	*
	* @note Copy and move operations are disabled to prevent accidental
	*       duplication of the underlying GLFW/Vulkan window resources.
	*/
	class VKWindow : public Window
	{
	public:
		uint16_t width_;  ///< Current window width in pixels.
		uint16_t height_; ///< Current window height in pixels.

    /**
     * @brief Constructs and opens a window of the given dimensions.
     *
     * Initializes GLFW, creates the native window, and registers the
     * framebuffer-resize callback.
     *
     * @param w  Desired window width in pixels.
     * @param h  Desired window height in pixels.
     */
    VKWindow(const uint16_t w, const uint16_t h);

    /**
     * @brief Destroys the GLFW window and terminates GLFW.
     */
    ~VKWindow();

    VKWindow(const VKWindow&) = delete;
    VKWindow& operator=(const VKWindow&) = delete;
    VKWindow(VKWindow&&) noexcept = delete;
    VKWindow& operator=(VKWindow&&) noexcept = delete;

    /**
     * @brief Submits the rendered frame to the screen.
     *
     * Called once per frame after all draw commands have been recorded.
     */
    void render() override;

    /**
     * @brief Returns whether the user has requested to close the window.
     *
     * @return True if the window close flag is set.
     */
    bool isClosed() override;

    /**
     * @brief Releases all GLFW and window-related resources.
     *
     * Should be called before the Vulkan device is destroyed.
     */
    void cleanup() override;

    /**
     * @brief Returns the underlying GLFW window handle.
     *
     * Used by Vulkan surface creation and input polling.
     *
     * @return Pointer to the native GLFWwindow.
     */
    GLFWwindow* getNativeWindow() override { return glfw_window_; }

    /**
     * @brief Swaps the front and back buffers (no-op in Vulkan; kept for API parity).
     *
     * Vulkan presentation is handled by the swapchain; this method exists
     * for interface compatibility.
     */
    void swap() override;

    /**
     * @brief Polls and dispatches pending OS/input events.
     *
     * Should be called once at the beginning of each frame.
     */
    void processEvents() override;

    /**
     * @brief Returns the current window height in pixels.
     * @return Window height as an unsigned 16-bit integer.
     */
    u16 getHeight() override;

    /**
     * @brief Returns the current window width in pixels.
     * @return Window width as an unsigned 16-bit integer.
     */
    u16 getWidth() override;

    /**
     * @brief Flag set by the GLFW framebuffer-resize callback.
     *
     * When true, the render loop must recreate the swapchain before
     * the next draw call to match the new surface dimensions.
     */
    bool framebufferResized_ = false;

  private:
    GLFWwindow* glfw_window_; ///< Opaque GLFW window handle.
	};

}
