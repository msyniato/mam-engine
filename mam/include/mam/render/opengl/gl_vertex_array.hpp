#pragma once

#include "render/api/vertex_array.hpp"

namespace mam {

  /**
   * @brief OpenGL implementation of a Vertex Array Object (VAO).
   *
   * Uses Direct State Access (DSA) where possible and ensures that
   * the element array buffer binding is recorded inside the VAO state.
   */
  class GLVertexArray : public VertexArray
  {
  public:
    /// Construct a new VAO and allocate the underlying OpenGL object.
    GLVertexArray();

    /// Destroy the VAO and release GPU resources.
    ~GLVertexArray();

    /// Bind the VAO for rendering.
    void bind() const override;

    /// Unbind the VAO.
    void unBind() const override;

    /// Delete the underlying OpenGL VAO.
    void release() const override;

    /**
     * @brief Attach a vertex buffer and record a vertex attribute pointer.
     *
     * The buffer is bound to GL_ARRAY_BUFFER before calling
     * glVertexAttribPointer so the correct buffer is recorded in the VAO state.
     *
     * @param vertexBuffer Vertex buffer to attach.
     * @param index        Attribute location index.
     * @param count        Number of components per attribute.
     * @param type         Component data type.
     * @param normalized   Whether to normalise fixed-point values.
     * @param stride       Byte stride between consecutive vertex attributes.
     * @param offset       Byte offset of the first attribute within the buffer.
     */
    void addVertexBuffer(VertexBuffer* vertexBuffer,
                         unsigned int  index,
                         int           count,
                         UniformType   type,
                         bool          normalized,
                         int           stride,
                         const void*   offset) override;

    /**
     * @brief Attach an instanced vertex buffer with a per-instance attribute divisor.
     * @param vertexBuffer Vertex buffer containing per-instance data.
     * @param index        Attribute location index.
     * @param count        Number of components per attribute.
     * @param type         Component data type.
     * @param stride       Byte stride between consecutive instances.
     * @param offset       Byte offset of the first attribute.
     * @param divisor      Number of instances to advance per attribute (default 1).
     */
    void addInstanceBuffer(VertexBuffer* vertexBuffer,
                           unsigned int  index,
                           int           count,
                           UniformType   type,
                           int           stride,
                           const void*   offset,
                           unsigned int  divisor = 1) override;

    /**
     * @brief Attach a mat4 instance buffer using four consecutive attribute slots.
     *
     * Each column of the mat4 is recorded as a separate vec4 attribute
     * (slots baseIndex through baseIndex + 3).
     *
     * @param vertexBuffer Vertex buffer containing per-instance mat4 data.
     * @param baseIndex    First of four consecutive attribute location indices.
     * @param divisor      Instance divisor (default 1).
     */
    void addInstanceBufferMat4(VertexBuffer* vertexBuffer,
                               unsigned int  baseIndex,
                               unsigned int  divisor = 1) override;

    /**
     * @brief Attach an index buffer and record it inside the VAO state.
     *
     * The index buffer is bound to GL_ELEMENT_ARRAY_BUFFER while this VAO
     * is active, so glDrawElements can locate the indices at draw time.
     *
     * @param indexBuffer Index buffer to attach.
     */
    void setIndexBuffer(IndexBuffer* indexBuffer) override;
  };

} // namespace mam
