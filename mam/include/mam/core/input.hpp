#pragma once

#include "common/common.hpp"

namespace mam {

  class GraphicsContext;

  /**
   * @brief Keyboard and mouse input manager.
   *
   * Queries the GLFW window state for key, mouse button, and scroll events.
   */
  class InputManager
  {
  public:
    /**
     * @brief Construct the input manager bound to a GLFW window.
     * @param window Native GLFW window to poll events from.
     */
    InputManager(GLFWwindow* window);
    ~InputManager() = default;

    /// Initialise the input manager (registers scroll callback).
    void init();

    /**
     * @brief Returns true if the key was pressed this frame.
     * @param key Key code to test.
     */
    bool isKeyPressed(KeyCode key);

    /**
     * @brief Returns true if the key was released this frame.
     * @param key Key code to test.
     */
    bool isKeyReleased(KeyCode key);

    /**
     * @brief Returns true if the key is held and generating repeat events.
     * @param key Key code to test.
     */
    bool isKeyRepeated(KeyCode key);

    /**
     * @brief Returns true if the mouse button was pressed this frame.
     * @param key Mouse button code to test.
     */
    bool isMouseButtonPressed(MouseCode key);

    /**
     * @brief Returns true if the mouse button was released this frame.
     * @param key Mouse button code to test.
     */
    bool isMouseButtonReleased(MouseCode key);

    /// Returns the vertical scroll offset accumulated since the last call.
    float getMouseScrollOffset();

    /**
     * @brief Show or hide the system cursor.
     * @param show True to show the cursor, false to hide and capture it.
     */
    void showMouse(bool show);

    /// Returns the current mouse position in window-space pixels.
    glm::vec2 mousePosition();

    /// Returns the current mouse X coordinate in window-space pixels.
    float mouseX();

    /// Returns the current mouse Y coordinate in window-space pixels.
    float mouseY();

  private:
    GLFWwindow* window_;                ///< Bound GLFW window.
    static float scrollOffsetY_;        ///< Accumulated vertical scroll offset.

    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
  };

} // namespace mam
