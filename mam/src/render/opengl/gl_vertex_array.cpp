
#include "render/opengl/gl_vertex_array.hpp"

namespace mam
{

  GLVertexArray::GLVertexArray()
  {
    glCreateVertexArrays(1, &id_);
  }

  GLVertexArray::~GLVertexArray()
  {
    release();
  }

  void GLVertexArray::bind() const
  {
    glBindVertexArray(id_);
  }

  void GLVertexArray::unBind() const
  {
    glBindVertexArray(0);
  }

  void GLVertexArray::release() const
  {
    glDeleteVertexArrays(1, &id_);
  }

  void GLVertexArray::addVertexBuffer(VertexBuffer* vertexBuffer,
                                      unsigned int index, int count, UniformType type, bool normalized, int stride, const void* offset)
  {
    const GLuint bufferId  = vertexBuffer->getBufferID();
    const GLuint relOffset = static_cast<GLuint>(reinterpret_cast<uintptr_t>(offset));

    glEnableVertexArrayAttrib(id_, index);
    glVertexArrayVertexBuffer(id_, index, bufferId, 0, stride);
    glVertexArrayAttribFormat(id_, index, count, GL_FLOAT, normalized ? GL_TRUE : GL_FALSE, relOffset);
    glVertexArrayAttribBinding(id_, index, index);
  }

  void GLVertexArray::addInstanceBuffer(VertexBuffer* vertexBuffer,
    unsigned int index, int count,
    UniformType type, int stride,
    const void* offset, unsigned int divisor)
  {
    const GLuint bufferId  = vertexBuffer->getBufferID();
    const GLuint relOffset = static_cast<GLuint>(reinterpret_cast<uintptr_t>(offset));

    glEnableVertexArrayAttrib(id_, index);
    glVertexArrayVertexBuffer(id_, index, bufferId, 0, stride);
    glVertexArrayAttribFormat(id_, index, count, GL_FLOAT, GL_FALSE, relOffset);
    glVertexArrayAttribBinding(id_, index, index);
    glVertexArrayBindingDivisor(id_, index, divisor);
  }

  void GLVertexArray::addInstanceBufferMat4(VertexBuffer* vertexBuffer,
    unsigned int baseIndex, unsigned int divisor)
  {
    const GLuint bufferId   = vertexBuffer->getBufferID();
    constexpr int vecSize   = sizeof(glm::vec4);
    constexpr int mat4Size  = sizeof(glm::mat4);

    for (int i = 0; i < 4; ++i) {
      const GLuint slot = baseIndex + i;
      glEnableVertexArrayAttrib(id_, slot);
      glVertexArrayVertexBuffer(id_, slot, bufferId, 0, mat4Size);
      glVertexArrayAttribFormat(id_, slot, 4, GL_FLOAT, GL_FALSE, static_cast<GLuint>(vecSize * i));
      glVertexArrayAttribBinding(id_, slot, slot);
      glVertexArrayBindingDivisor(id_, slot, divisor);
    }
  }

  void GLVertexArray::setIndexBuffer(IndexBuffer* indexBuffer)
  {
    indexBuffer_ = indexBuffer;
    if (indexBuffer)
      glVertexArrayElementBuffer(id_, indexBuffer->getBufferID());
  }

}
