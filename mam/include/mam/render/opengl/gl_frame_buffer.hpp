#pragma once

#include "render/api/frame_buffer.hpp"

namespace mam
{
  /**
   * @brief OpenGL implementation of a framebuffer
   *
   * Handles creation, binding, resizing, and management of OpenGL framebuffer objects.
   */
  class GLFramebuffer : public FrameBuffer
  {
  public:
    /**
     * @brief Construct a GLFramebuffer with given specifications
     * @param spec Framebuffer specifications (size, attachments, etc.)
     */
    GLFramebuffer(const FrameBufferSpec& spec);

    /**
     * @brief Destructor, cleans up OpenGL framebuffer resources
     */
    ~GLFramebuffer() override;

    /**
     * @brief Bind this framebuffer for rendering
     */
    void bind() const override;

    /**
     * @brief Unbind the framebuffer (binds default framebuffer)
     */
    void unbind() const override;

    /**
     * @brief Resize the framebuffer and its attachments
     * @param width New width
     * @param height New height
     */
    void resize(u32 width, u32 height) override;

  protected:
    /**
     * @brief Recreate or invalidate the framebuffer resources
     *        Called internally after resize or initialization
     */
    void invalidate() override;
  };
}