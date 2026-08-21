#pragma once

#include "render/api/buffer.hpp"

namespace mam {

  /**
   * @brief OpenGL implementation of a vertex buffer (VBO).
   *
   * Uses Direct State Access (DSA) for all GPU operations.
   */
  class GLVertexBuffer : public VertexBuffer {
  public:
    /**
     * @brief Construct an empty vertex buffer with a given byte capacity.
     * @param count Capacity in bytes.
     */
    GLVertexBuffer(unsigned int count);

    /**
     * @brief Construct a vertex buffer and upload initial data.
     * @param vertex Pointer to vertex data.
     * @param count  Number of bytes to upload.
     */
    GLVertexBuffer(void* vertex, unsigned int count);

    ~GLVertexBuffer() override;

    /// Bind the buffer to GL_ARRAY_BUFFER.
    void bind() override;

    /**
     * @brief Upload data to the GPU buffer.
     * @param data   Pointer to source data.
     * @param size   Number of bytes to upload.
     * @param offset Byte offset within the buffer (default 0).
     */
    void uploadData(const void* data, unsigned int size, unsigned int offset = 0) override;

    /// Delete the underlying GPU buffer.
    void release() override;
  };

  /**
   * @brief OpenGL implementation of an index buffer (EBO/IBO).
   *
   * Uses Direct State Access (DSA) for all GPU operations.
   */
  class GLIndexBuffer : public IndexBuffer {
  public:
    /**
     * @brief Construct an empty index buffer with a given byte capacity.
     * @param size Capacity in bytes.
     */
    GLIndexBuffer(unsigned int size);

    /**
     * @brief Construct an index buffer and upload initial index data.
     * @param index Pointer to index data.
     * @param size  Number of bytes to upload.
     */
    GLIndexBuffer(void* index, unsigned int size);

    ~GLIndexBuffer() override;

    /// Bind the buffer to GL_ELEMENT_ARRAY_BUFFER.
    void bind() override;

    /**
     * @brief Upload index data to the GPU buffer.
     * @param data   Pointer to source index data.
     * @param size   Number of bytes to upload.
     * @param offset Byte offset within the buffer (default 0).
     */
    void uploadData(const void* data, unsigned int size, unsigned int offset = 0) override;

    /// Delete the underlying GPU buffer.
    void release() override;
  };

  /**
   * @brief OpenGL Shader Storage Buffer Object (SSBO).
   *
   * Allows shaders to read and write structured buffer data on the GPU.
   * Uses Direct State Access (DSA) for uploads.
   */
  class GLShaderStorageBuffer : public ShaderStorageBuffer {
  public:
    /**
     * @brief Construct and allocate the SSBO at the given binding point.
     * @param binding_point Binding point index used in shader layout qualifiers.
     */
    GLShaderStorageBuffer(u32 binding_point);

    ~GLShaderStorageBuffer() override;

    /// Bind the SSBO to its binding point.
    void bind() override;

    /// No-op unbind (binding points are managed by the pipeline state).
    void unBind() override;

    /**
     * @brief Upload data to the SSBO.
     * @param data   Pointer to source data.
     * @param size   Number of bytes to upload.
     * @param offset Byte offset within the buffer.
     */
    void uploadData(const void* data, u16 size, u8 offset) override;

    /// Delete the underlying GPU buffer.
    void release() override;
  };

  /**
   * @brief OpenGL G-Buffer for deferred rendering.
   *
   * Manages a framebuffer with three RGBA16F colour attachments
   * (position, normal, colour + specular) and a depth renderbuffer.
   * All objects are created and configured via Direct State Access (DSA).
   */
  class GLGBuffer : public GBuffer {
  public:
    /**
     * @brief Construct a G-Buffer sized to the given screen dimensions.
     * @param screen_size Viewport size as (width, height).
     */
    GLGBuffer(glm::vec2 screen_size);

    ~GLGBuffer();

    /// Bind the G-Buffer framebuffer as the active render target.
    void bind() override;

    /// Clear all colour attachments and the depth buffer.
    void cleanTextures() override;

    /**
     * @brief Bind all G-Buffer textures to texture units 0–2 for the shading pass.
     *
     * Unit 0 = position, Unit 1 = normals, Unit 2 = colour/specular.
     */
    void bindTextures() override;

    /// Restore the default framebuffer.
    void unBind() override;

    /// Delete the framebuffer, all colour textures, and the depth renderbuffer.
    void release() override;
  };

} // namespace mam
