#pragma once

#include "common/common.hpp"
#include "render/api/buffer.hpp"

namespace mam {

  class VertexBuffer;
  class IndexBuffer;

  /**
   * @brief Abstract Vertex Array Object (VAO).
   *
   * Manages the binding of one or more vertex buffers, an optional index buffer,
   * and records vertex attribute layout for rendering.
   */
  class VertexArray {
  public:
    VertexArray() = default;
    virtual ~VertexArray() = default;

    /// Bind the VAO for rendering.
    virtual void bind() const = 0;

    /// Unbind the VAO.
    virtual void unBind() const = 0;

    /// Release the underlying GPU resource.
    virtual void release() const = 0;

    /**
     * @brief Attach a vertex buffer and record a vertex attribute pointer.
     * @param vertexBuffer Vertex buffer to attach.
     * @param index        Attribute location index.
     * @param count        Number of components per attribute.
     * @param type         Component data type.
     * @param normalized   Whether to normalise fixed-point values.
     * @param stride       Byte stride between consecutive attributes.
     * @param offset       Byte offset of the first attribute in the buffer.
     */
    virtual void addVertexBuffer(VertexBuffer* vertexBuffer,
                                 unsigned int  index      = 0,
                                 int           count      = 0,
                                 UniformType   type       = UniformType::Float1,
                                 bool          normalized = false,
                                 int           stride     = 0,
                                 const void*   offset     = 0) = 0;

    /**
     * @brief Attach an instanced vertex buffer with a per-instance divisor.
     * @param vertexBuffer Vertex buffer containing per-instance data.
     * @param index        Attribute location index.
     * @param count        Number of components per attribute.
     * @param type         Component data type.
     * @param stride       Byte stride between consecutive instances.
     * @param offset       Byte offset of the first attribute.
     * @param divisor      Number of instances to advance per attribute (default 1).
     */
    virtual void addInstanceBuffer(VertexBuffer* vertexBuffer,
                                   unsigned int  index,
                                   int           count,
                                   UniformType   type,
                                   int           stride,
                                   const void*   offset,
                                   unsigned int  divisor = 1) = 0;

    /**
     * @brief Attach a mat4 instance buffer, consuming four consecutive attribute slots.
     * @param vertexBuffer Vertex buffer containing per-instance mat4 data.
     * @param baseIndex    First of four consecutive attribute location indices.
     * @param divisor      Instance divisor (default 1).
     */
    virtual void addInstanceBufferMat4(VertexBuffer* vertexBuffer,
                                       unsigned int  baseIndex,
                                       unsigned int  divisor = 1) = 0;

    /**
     * @brief Attach an index buffer to this VAO.
     * @param indexBuffer Index buffer to record.
     */
    virtual void setIndexBuffer(IndexBuffer* indexBuffer) = 0;

  protected:
    VertexBuffer* vertexBuffers_; ///< Attached vertex buffer (first/primary).
    IndexBuffer*  indexBuffer_;   ///< Attached index buffer.
    uint32_t      id_;            ///< GPU VAO identifier.
  };

} // namespace mam
