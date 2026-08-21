#include "render/opengl/gl_frame_buffer.hpp"
#include "render/opengl/gl_texture.hpp"

namespace mam
{
  GLFramebuffer::GLFramebuffer(const FrameBufferSpec& spec)
    : FrameBuffer(spec)
  {
    invalidate();
  }

  GLFramebuffer::~GLFramebuffer()
  {
    if (fbo_ != 0)
    {
      glDeleteFramebuffers(1, &fbo_);
      fbo_ = 0;
    }
  }

  void GLFramebuffer::invalidate()
  {
    if (fbo_ != 0)
    {
      glDeleteFramebuffers(1, &fbo_);
      fbo_ = 0;
      colorAttachments_.clear();
      depthAttachment_.reset();
    }

    glCreateFramebuffers(1, &fbo_);

    bool hasColor = !spec_.colorFormats.empty();

    if (hasColor)
    {
      colorAttachments_.resize(spec_.colorFormats.size());

      std::vector<GLenum> drawBuffers;
      drawBuffers.reserve(spec_.colorFormats.size());

      for (u32 i = 0; i < spec_.colorFormats.size(); ++i)
      {
        Texture::TextureSettings ts;
        ts.format = spec_.colorFormats[i];
        ts.minFilter = Filter::LINEAR;
        ts.magFilter = Filter::LINEAR;
        ts.wrap = Wrap::CLAMP_TO_EDGE;

        auto tex = std::make_shared<GLTexture>(spec_.width, spec_.height, ts);
        colorAttachments_[i] = tex;

        glNamedFramebufferTexture(
          fbo_,
          GL_COLOR_ATTACHMENT0 + i,
          tex->id(),
          0
        );

        drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
      }

      glNamedFramebufferDrawBuffers(fbo_, (GLsizei)drawBuffers.size(), drawBuffers.data());
    }
    else
    {
      glNamedFramebufferDrawBuffer(fbo_, GL_NONE);
      glNamedFramebufferReadBuffer(fbo_, GL_NONE);
    }

    if (spec_.hasDepth)
    {
      Texture::TextureSettings ts;
      ts.format = spec_.depthFormat;
      ts.minFilter = Filter::NEAREST;
      ts.magFilter = Filter::NEAREST;
      ts.wrap = Wrap::CLAMP_TO_EDGE;

      auto depthTex = std::make_shared<GLTexture>(spec_.width, spec_.height, ts);
      depthAttachment_ = depthTex;

      GLenum attachment = GL_DEPTH_ATTACHMENT;
      if (spec_.depthFormat == Format::DEPTH24STENCIL8)
        attachment = GL_DEPTH_STENCIL_ATTACHMENT;

      glNamedFramebufferTexture(fbo_, attachment, depthTex->id(), 0);
    }
    GLenum status = glCheckNamedFramebufferStatus(fbo_, GL_FRAMEBUFFER);
  }

  void GLFramebuffer::bind() const
  {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, (GLsizei)spec_.width, (GLsizei)spec_.height);
  }

  void GLFramebuffer::unbind() const
  {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  void GLFramebuffer::resize(u32 width, u32 height)
  {
    if (width == 0 || height == 0) return;
    if (width == spec_.width && height == spec_.height) return; 
    spec_.width = width;
    spec_.height = height;
    invalidate();
  }
}
