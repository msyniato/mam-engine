#pragma once

#include "common/common.hpp"

namespace mam {

  /**
   * @brief Describes a single vertex attribute within a vertex buffer.
   */
  struct VertexBufferElement
  {
    std::string name_;      ///< Name of the attribute.
    UniformType type_;      ///< Data type of the attribute.
    uint32_t    size_;      ///< Size in bytes.
    uint32_t    offset_;    ///< Byte offset from the start of a vertex.
    bool        normalized_; ///< Whether the value should be normalised on upload.

    VertexBufferElement() {}

    /**
     * @brief Construct a vertex buffer element.
     * @param type       Data type of the attribute.
     * @param name       Attribute name.
     * @param normalized Whether the value should be normalised (default false).
     */
    VertexBufferElement(UniformType type, std::string name, bool normalized = false)
      : type_(type), name_(name), size_(UniformTypeSize(type)), offset_(0), normalized_(normalized) {}

    /**
     * @brief Returns the number of scalar components in this attribute.
     *
     * For example, Float3 returns 3 and Mat4 returns 16.
     * @return Component count.
     */
    uint32_t GetComponentCount() const {
      switch (type_)
      {
      case UniformType::Byte1:   return 1;
      case UniformType::UByte1:  return 1;
      case UniformType::Byte2:   return 2;
      case UniformType::UByte2:  return 2;
      case UniformType::Byte3:   return 3;
      case UniformType::UByte3:  return 3;
      case UniformType::Byte4:   return 4;
      case UniformType::UByte4:  return 4;

      case UniformType::Short1:  return 1;
      case UniformType::UShort1: return 1;
      case UniformType::Short2:  return 2;
      case UniformType::UShort2: return 2;
      case UniformType::Short3:  return 3;
      case UniformType::UShort3: return 3;
      case UniformType::Short4:  return 4;
      case UniformType::UShort4: return 4;

      case UniformType::Float1:  return 1;
      case UniformType::Float2:  return 2;
      case UniformType::Float3:  return 3;
      case UniformType::Float4:  return 4;

      case UniformType::Double1: return 1;
      case UniformType::Double2: return 2;
      case UniformType::Double3: return 3;
      case UniformType::Double4: return 4;

      case UniformType::UInt1:   return 1;
      case UniformType::UInt2:   return 2;
      case UniformType::UInt3:   return 3;
      case UniformType::UInt4:   return 4;
      case UniformType::Int1:    return 1;
      case UniformType::Int2:    return 2;
      case UniformType::Int3:    return 3;
      case UniformType::Int4:    return 4;

      case UniformType::Mat2:    return 2 * 2;
      case UniformType::Mat3:    return 3 * 3;
      case UniformType::Mat4:    return 4 * 4;

      case UniformType::Bool:    return 1;
      }
    }
  };

  /**
   * @brief Describes the interleaved layout of a vertex buffer.
   */
  class VertexBufferLayout
  {
  public:
    VertexBufferLayout() {}

    /**
     * @brief Construct a layout from a list of elements.
     *
     * Automatically computes per-element offsets and the total stride.
     * @param element Initializer list of vertex buffer elements.
     */
    VertexBufferLayout(const std::initializer_list<VertexBufferElement>& element)
      : elements_(element) {
      CalculateOffsetStride();
    }

    /// Returns the list of vertex buffer elements.
    inline const std::vector<VertexBufferElement>& getElements() const { return elements_; }

    uint32_t stride = 0; ///< Total vertex stride in bytes.

    std::vector<VertexBufferElement>::iterator begin() { return elements_.begin(); }
    std::vector<VertexBufferElement>::iterator end()   { return elements_.end(); }

  private:
    void CalculateOffsetStride()
    {
      uint32_t offset = 0;
      stride = 0;
      for (auto& element : elements_)
      {
        element.offset_ = offset;
        offset += element.size_;
        stride += element.size_;
      }
    }

    std::vector<VertexBufferElement> elements_;
  };

  /**
   * @brief Abstract GPU vertex buffer.
   */
  class VertexBuffer
  {
  public:
    VertexBuffer() = default;
    virtual ~VertexBuffer() = default;

    /// Bind the buffer to the graphics pipeline.
    virtual void bind() = 0;

    /**
     * @brief Upload data to the GPU buffer.
     * @param data   Pointer to the source data.
     * @param size   Number of bytes to upload.
     * @param offset Byte offset within the buffer (default 0).
     */
    virtual void uploadData(const void* data, unsigned int size, unsigned int offset = 0) = 0;

    /// Release the underlying GPU resource.
    virtual void release() = 0;

    /**
     * @brief Set the vertex layout descriptor.
     * @param layout Vertex buffer layout.
     */
    void setLayout(const VertexBufferLayout& layout) { layout_ = layout; }

    /// Returns the vertex layout descriptor.
    VertexBufferLayout& getLayout() { return layout_; }

    /// Returns the number of vertices stored in this buffer.
    u32 count() { return count_; }

    /// Returns the underlying GPU buffer ID.
    u32 getBufferID() const { return id_; }

  protected:
    u32                id_;      ///< GPU buffer identifier.
    u32                count_;   ///< Number of vertices.
    VertexBufferLayout layout_;  ///< Vertex layout descriptor.
  };

  /**
   * @brief Abstract GPU index buffer.
   */
  class IndexBuffer
  {
  public:
    IndexBuffer() = default;
    virtual ~IndexBuffer() = default;

    /// Bind the buffer to the graphics pipeline.
    virtual void bind() = 0;

    /**
     * @brief Upload index data to the GPU buffer.
     * @param data   Pointer to the source index data.
     * @param size   Number of bytes to upload.
     * @param offset Byte offset within the buffer (default 0).
     */
    virtual void uploadData(const void* data, unsigned int size, unsigned int offset = 0) = 0;

    /// Release the underlying GPU resource.
    virtual void release() = 0;

    /// Returns the buffer size in bytes.
    u32 size()  { return size_; }

    /// Returns the number of indices stored in this buffer.
    u32 count() { return count_; }

    /// Returns the underlying GPU buffer ID.
    u32 getBufferID() const { return id_; }

  protected:
    u32 id_;    ///< GPU buffer identifier.
    u32 size_;  ///< Buffer size in bytes.
    u32 count_; ///< Number of indices.
  };

  /**
   * @brief Abstract Shader Storage Buffer Object (SSBO).
   */
  class ShaderStorageBuffer {
  public:
    ShaderStorageBuffer() = default;
    virtual ~ShaderStorageBuffer() = default;

    /// Bind the SSBO to its binding point.
    virtual void bind() = 0;

    /// Unbind the SSBO.
    virtual void unBind() = 0;

    /**
     * @brief Upload or update data in the SSBO.
     * @param data   Pointer to source data.
     * @param size   Number of bytes to upload.
     * @param offset Byte offset within the buffer.
     */
    virtual void uploadData(const void* data, u16 size, u8 offset) = 0;

    /// Release the underlying GPU resource.
    virtual void release() = 0;

  protected:
    u32 id_;                   ///< GPU buffer identifier.
    u32 block_index_;          ///< Shader block index.
    u32 binding_point_index_;  ///< Binding point index.
    u32 size_;                 ///< Buffer size in bytes.
  };

  /**
   * @brief Abstract G-Buffer used for deferred rendering.
   *
   * Stores multiple render targets (position, normal, colour/specular)
   * in a single framebuffer object.
   */
  class GBuffer {
  public:
    GBuffer() = default;
    ~GBuffer() = default;

    /// Bind the G-Buffer framebuffer as the active render target.
    virtual void bind() = 0;

    /// Clear all attached render target textures.
    virtual void cleanTextures() = 0;

    /// Bind the G-Buffer textures for the shading pass.
    virtual void bindTextures() = 0;

    /// Unbind the G-Buffer framebuffer, restoring the default target.
    virtual void unBind() = 0;

    /// Release all GPU resources held by this G-Buffer.
    virtual void release() = 0;

  protected:
    u32 id_;    ///< Framebuffer object identifier.
    u32 pos_;   ///< Position render target texture.
    u32 norm_;  ///< Normal render target texture.
    u32 color_; ///< Colour + specular render target texture.
    u32 dbo_;   ///< Depth renderbuffer identifier.
  };

} // namespace mam
