
#pragma once

#include "render/api/vertex_array.hpp"

namespace mam {

  /**
   * @brief Vulkan implementation of a vertex array object (VAO)
   *
   * Manages vertex and index buffers for rendering with Vulkan.
   */
  class VKVertexArray : public VertexArray {
  public:

    /**
     * @brief Constructor
     */
    VKVertexArray() {};

    /**
     * @brief Destructor
     */
    ~VKVertexArray() {};

    VKVertexArray(const VKVertexArray&) = delete;
    VKVertexArray& operator=(const VKVertexArray&) = delete;
    VKVertexArray(VKVertexArray&&) noexcept = delete;
    VKVertexArray& operator=(VKVertexArray&&) noexcept = delete;

    /**
     * @brief Bind the vertex array for rendering
     */
    void bind() const override;

    /**
     * @brief Unbind the vertex array
     */
    void unBind() const override;

    /**
     * @brief Releases GPU resources owned by this vertex array.
     */
    virtual void release() const override;

    /**
     * @brief Adds a per-vertex attribute binding to the vertex array.
     *
     * @param vertexBuffer  Source vertex buffer containing the attribute data.
     * @param index         Shader attribute location index.
     * @param count         Number of components (e.g. 3 for a vec3).
     * @param type          Data type of each component (see UniformType).
     * @param normalized    Whether integer data should be normalized to [0,1] or [-1,1].
     * @param stride        Byte stride between consecutive vertex elements.
     * @param offset        Byte offset of the first element within the buffer.
     */
		void addVertexBuffer(VertexBuffer* vertexBuffer,
			unsigned int index, int count, UniformType type, bool normalized, int stride, const void* offset) override;

    /**
    * @brief Adds a per-instance attribute binding to the vertex array.
    *
    * Configures a vertex buffer to advance once per instance rather than
    * once per vertex, enabling hardware instancing.
    *
    * @param vertexBuffer  Source vertex buffer containing instance data.
    * @param index         Shader attribute location index.
    * @param count         Number of components per attribute element.
    * @param type          Data type of each component (see UniformType).
    * @param stride        Byte stride between consecutive instance elements.
    * @param offset        Byte offset of the first element within the buffer.
    * @param divisor       Attribute divisor; defaults to 1 (advance once per instance).
    */
    void addInstanceBuffer(VertexBuffer* vertexBuffer,
      unsigned int index, int count,UniformType type, int stride,
      const void* offset, unsigned int divisor = 1) override;

    /**
    * @brief Adds a 4x4 matrix instance buffer using four consecutive attribute slots.
    *
    * A mat4 occupies four vec4 attribute locations. This helper automatically
    * configures all four consecutive bindings starting at @p baseIndex.
    *
    * @param vertexBuffer  Source vertex buffer containing mat4 instance data.
    * @param baseIndex     First shader attribute location (occupies baseIndex to baseIndex+3).
    * @param divisor       Attribute divisor; defaults to 1 (advance once per instance).
    */
    void addInstanceBufferMat4(VertexBuffer* vertexBuffer,
      unsigned int baseIndex, unsigned int divisor = 1) override;

    /**
     * @brief Sets the index buffer used for indexed draw calls.
     *
     * @param indexBuffer  Index buffer to bind. Ownership is not transferred.
     */
		void setIndexBuffer(IndexBuffer* indexBuffer) override;

  };

}