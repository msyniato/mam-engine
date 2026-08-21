#pragma once

#include "ui/imgui_wrapper.hpp"
#include "vulkan/vulkan.h"

namespace mam {

  class GraphicsContext;
  class VKDevice;
  class FrameBuffer;

  /**
   * @brief Vulkan back-end implementation of ImGUIWrapper.
   *
   * Uses the ImGui Vulkan + GLFW backend and manages a per-frame Vulkan
   * descriptor set for blitting the scene framebuffer into the ImGui viewport.
   * Font upload to the GPU is handled via a one-time command buffer submission.
   */
  class VKImGUIWrapper : public ImGUIWrapper {
  public:
    VKImGUIWrapper()          = default;
    ~VKImGUIWrapper() override = default;

    /// Begin a new ImGui frame using the Vulkan/GLFW backend.
    void beginFrame() override;

    /// End the current ImGui frame and record ImGui draw commands into the active command buffer.
    void endFrame() override;

    /**
     * @brief Render the scene viewport by blitting the Vulkan framebuffer via a descriptor set.
     * @param context Application context providing access to the renderer.
     */
    void renderViewport(Context& context) override;

  private:
    /**
     * @brief Initialise the ImGui Vulkan + GLFW backend and upload fonts.
     * @param window Native GLFW window handle.
     */
    void initImplementation(GLFWwindow* window) override;

    /// Shut down the ImGui Vulkan backend and release descriptor resources.
    void shutdownImplementation() override;

    /// Upload ImGui font atlas to a Vulkan texture via a temporary command buffer.
    void uploadFonts();

    /**
     * @brief Recreate the ImGui image descriptor set for the given framebuffer.
     * @param vkDevice Vulkan device wrapper.
     * @param fb       Framebuffer whose colour attachment to display in the viewport.
     */
    void rebuildViewportDescriptor(VKDevice* vkDevice, FrameBuffer* fb);

    VkDescriptorSet viewportDescriptorSet_ = VK_NULL_HANDLE; ///< Descriptor set for the viewport image.
    uint32_t        lastViewportW_         = 0;              ///< Width of the last built viewport descriptor.
    uint32_t        lastViewportH_         = 0;              ///< Height of the last built viewport descriptor.
  };

} // namespace mam
