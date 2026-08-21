#pragma once

#include "render/api/texture.hpp"

namespace mam
{
  /**
   * @brief Specification for creating a framebuffer.
   *
   * Defines size, color attachment formats, and depth attachment options.
   */
  struct FrameBufferSpec
  {
    /// Width of the framebuffer in pixels
    u32 width = 0;

    /// Height of the framebuffer in pixels
    u32 height = 0;

    /// Formats for color attachments
    std::vector<mam::Format> colorFormats;

    /// Whether the framebuffer has a depth attachment
    bool hasDepth = true;

    /// Format of the depth attachment (if any)
    mam::Format depthFormat = mam::Format::DEPTH24STENCIL8;
  };

  /**
   * @brief Abstract framebuffer class.
   *
   * Manages color and depth attachments, supports binding, resizing,
   * and invalidation. Specific APIs (OpenGL, Vulkan, etc.) should
   * implement this interface.
   */
  class FrameBuffer
  {
  public:
    /**
     * @brief Constructs a framebuffer with a given specification.
     * @param spec Framebuffer specification
     */
    explicit FrameBuffer(const FrameBufferSpec& spec)
      : spec_(spec)
    {
    }

    /// Virtual destructor
    virtual ~FrameBuffer() = default;

    /// Bind the framebuffer for rendering
    virtual void bind() const = 0;

    /// Unbind the framebuffer
    virtual void unbind() const = 0;

    /**
     * @brief Resize the framebuffer and its attachments.
     * @param width New width in pixels
     * @param height New height in pixels
     */
    virtual void resize(u32 width, u32 height) = 0;

    /// Returns the framebuffer specification
    const FrameBufferSpec& spec() const { return spec_; }

    /// Returns all color attachments
    const std::vector<std::shared_ptr<Texture>>& getColorAttachments() const { return colorAttachments_; }

    /// Returns the depth attachment
    std::shared_ptr<Texture> getDepthAttachment() const { return depthAttachment_; }

    /// Returns the internal framebuffer ID
    u32 id() const { return fbo_; }

  protected:
    /// Invalidate and recreate the framebuffer (API-specific implementation)
    virtual void invalidate() = 0;

    FrameBufferSpec spec_;                                      ///< Framebuffer configuration
    u32 fbo_ = 0;                                               ///< GPU framebuffer ID

    std::vector<std::shared_ptr<Texture>> colorAttachments_;    ///< Color attachment textures
    std::shared_ptr<Texture> depthAttachment_;                 ///< Depth attachment texture
  };
}