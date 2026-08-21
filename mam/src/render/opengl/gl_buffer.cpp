
#include "render/opengl/gl_buffer.hpp"

namespace mam
{

  GLVertexBuffer::GLVertexBuffer(unsigned int count)
  {
    count_ = count;
    glCreateBuffers(1, &id_);
    glNamedBufferData(id_, count, nullptr, GL_DYNAMIC_DRAW);
  }

  GLVertexBuffer::GLVertexBuffer(void* vertex, unsigned int count)
  {
    count_ = count;
    glCreateBuffers(1, &id_);
    glNamedBufferData(id_, count, vertex, GL_STATIC_DRAW);
  }

  GLVertexBuffer::~GLVertexBuffer()
  {
    release();
  }

  void GLVertexBuffer::bind()
  {
    glBindBuffer(GL_ARRAY_BUFFER, id_);
  }

  void GLVertexBuffer::uploadData(const void* data, unsigned int size, unsigned int offset)
  {
    glNamedBufferSubData(id_, offset, size, data);
  }

  void GLVertexBuffer::release()
  {
    glDeleteBuffers(1, &id_);
  }


  GLIndexBuffer::GLIndexBuffer(unsigned int size)
  {
    size_ = size;
    glCreateBuffers(1, &id_);
    glNamedBufferData(id_, size, nullptr, GL_DYNAMIC_DRAW);
  }

  GLIndexBuffer::GLIndexBuffer(void* index, unsigned int size)
  {
    size_ = size;
    glCreateBuffers(1, &id_);
    glNamedBufferData(id_, size, index, GL_STATIC_DRAW);
  }

  GLIndexBuffer::~GLIndexBuffer()
  {
    release();
  }

  void GLIndexBuffer::bind()
  {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_);
  }

  void GLIndexBuffer::uploadData(const void* data, unsigned int size, unsigned int offset)
  {
    glNamedBufferSubData(id_, offset, size, data);
  }

  void GLIndexBuffer::release()
  {
    glDeleteBuffers(1, &id_);
  }


  GLShaderStorageBuffer::GLShaderStorageBuffer(u32 binding_point)
  {
    binding_point_index_ = binding_point;
    glCreateBuffers(1, &id_);
    glNamedBufferData(id_, sizeof(GPULight) * kMaxGPULights, nullptr, GL_DYNAMIC_DRAW);
  }

  void GLShaderStorageBuffer::bind()
  {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding_point_index_, id_);
  }

  void GLShaderStorageBuffer::uploadData(const void* data, u16 size, u8 offset)
  {
    glNamedBufferSubData(id_, offset, size, data);
  }

  void GLShaderStorageBuffer::unBind() {}

  void GLShaderStorageBuffer::release()
  {
    glDeleteBuffers(1, &id_);
  }

  GLShaderStorageBuffer::~GLShaderStorageBuffer()
  {
    release();
  }

  GLGBuffer::GLGBuffer(glm::vec2 screen_size)
  {
    const GLsizei w = (GLsizei)screen_size.x;
    const GLsizei h = (GLsizei)screen_size.y;

    glCreateFramebuffers(1, &id_);

    glCreateTextures(GL_TEXTURE_2D, 1, &pos_);
    glTextureStorage2D(pos_, 1, GL_RGBA16F, w, h);
    glTextureParameteri(pos_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(pos_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glNamedFramebufferTexture(id_, GL_COLOR_ATTACHMENT0, pos_, 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &norm_);
    glTextureStorage2D(norm_, 1, GL_RGBA16F, w, h);
    glTextureParameteri(norm_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(norm_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glNamedFramebufferTexture(id_, GL_COLOR_ATTACHMENT1, norm_, 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &color_);
    glTextureStorage2D(color_, 1, GL_RGBA16F, w, h);
    glTextureParameteri(color_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(color_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glNamedFramebufferTexture(id_, GL_COLOR_ATTACHMENT2, color_, 0);

    const unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glNamedFramebufferDrawBuffers(id_, 3, attachments);

    glCreateRenderbuffers(1, &dbo_);
    glNamedRenderbufferStorage(dbo_, GL_DEPTH_COMPONENT, w, h);
    glNamedFramebufferRenderbuffer(id_, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, dbo_);
  }

  void GLGBuffer::bind()
  {
    glBindFramebuffer(GL_FRAMEBUFFER, id_);
  }

  void GLGBuffer::cleanTextures()
  {
    glClearBufferfv(GL_COLOR, 0, glm::value_ptr(glm::vec3(0.0f, 0.0f, 0.0f)));
    glClearBufferfv(GL_COLOR, 1, glm::value_ptr(glm::vec3(0.5f, 0.5f, 0.5f)));
    glClearBufferfv(GL_COLOR, 2, glm::value_ptr(glm::vec3(0.3f, 0.3f, 0.3f)));
    glClear(GL_DEPTH_BUFFER_BIT);
  }

  void GLGBuffer::bindTextures()
  {
    glBindTextureUnit(0, pos_);
    glBindTextureUnit(1, norm_);
    glBindTextureUnit(2, color_);
  }

  void GLGBuffer::unBind()
  {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  void GLGBuffer::release()
  {
    if (pos_)   { glDeleteTextures(1, &pos_);   pos_   = 0; }
    if (norm_)  { glDeleteTextures(1, &norm_);  norm_  = 0; }
    if (color_) { glDeleteTextures(1, &color_); color_ = 0; }
    if (dbo_)   { glDeleteRenderbuffers(1, &dbo_); dbo_ = 0; }
    if (id_)    { glDeleteFramebuffers(1, &id_);   id_  = 0; }
  }

  GLGBuffer::~GLGBuffer()
  {
    release();
  }

}
