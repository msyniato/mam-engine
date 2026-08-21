#pragma once

#include "ui/imgui_wrapper.hpp"

namespace mam {

  /**
   * @brief OpenGL back-end implementation of ImGUIWrapper.
   *
   * Uses the ImGui OpenGL3 + GLFW backend to begin and end ImGui frames.
   * All shared editor panels and layout logic are inherited from ImGUIWrapper.
   */
  class GLImGUIWrapper : public ImGUIWrapper {
  public:
    GLImGUIWrapper()          = default;
    ~GLImGUIWrapper() override = default;

    /// Begin a new ImGui frame using the OpenGL3/GLFW backend.
    void beginFrame() override;

    /// End the current ImGui frame and issue OpenGL draw calls.
    void endFrame() override;

  protected:
    /**
     * @brief Initialise the ImGui OpenGL3 + GLFW backend.
     * @param window Native GLFW window handle.
     */
    void initImplementation(GLFWwindow* window) override;

    /// Shut down the ImGui OpenGL3 + GLFW backend.
    void shutdownImplementation() override;
  };

} // namespace mam
