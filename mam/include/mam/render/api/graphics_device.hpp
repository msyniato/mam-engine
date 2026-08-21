#pragma once

#include "common/common.hpp"
#include "render/api/frame_buffer.hpp"

namespace mam {

  class ShaderStorageBuffer;
  class VertexBuffer;
  class IndexBuffer;
  class FrameBuffer;
  class VertexArray;
  class Renderer;
  class Pipeline;
  class GBuffer;
  class Window;
  class Shader;
  class Mesh;

  /**
   * @brief Abstract graphics device.
   *
   * Factory for all GPU resource objects (shaders, buffers, textures, VAOs, etc.)
   * and entry point for draw calls. Implementations exist for OpenGL and Vulkan.
   */
  class GraphicsDevice
  {
  public:
    GraphicsDevice() = default;
    virtual ~GraphicsDevice() = default;

    /// Create an empty shader object.
    virtual std::unique_ptr<Shader> createShader() = 0;

    /// Create an empty render pipeline.
    virtual std::unique_ptr<Pipeline> createPipeline() = 0;

    /**
     * @brief Create an empty vertex buffer with the given byte capacity.
     * @param count Capacity in bytes.
     */
    virtual std::unique_ptr<VertexBuffer> createVertexBuffer(unsigned int count = 0) = 0;

    /**
     * @brief Create a vertex buffer and upload initial data.
     * @param vertex Pointer to vertex data.
     * @param count  Number of bytes to upload.
     */
    virtual std::unique_ptr<VertexBuffer> createVertexBuffer(void* vertex, unsigned int count) = 0;

    /**
     * @brief Create an empty index buffer with the given byte capacity.
     * @param size Capacity in bytes.
     */
    virtual std::unique_ptr<IndexBuffer> createIndexBuffer(unsigned int size = 0) = 0;

    /**
     * @brief Create an index buffer and upload initial data.
     * @param index Pointer to index data.
     * @param size  Number of bytes to upload.
     */
    virtual std::unique_ptr<IndexBuffer> createIndexBuffer(void* index, unsigned int size) = 0;

    /**
     * @brief Create a framebuffer from a specification.
     * @param spec Framebuffer configuration (size, attachments).
     */
    virtual std::unique_ptr<FrameBuffer> createFrameBuffer(const FrameBufferSpec& spec) = 0;

    /// Create a Vertex Array Object.
    virtual std::unique_ptr<VertexArray> createVertexArray() = 0;

    /**
     * @brief Create a G-Buffer for deferred rendering.
     * @param screen_size Viewport dimensions as (width, height).
     */
    virtual std::unique_ptr<GBuffer> createGBuffer(glm::vec2 screen_size) = 0;

    /**
     * @brief Create a Shader Storage Buffer Object bound to a pipeline slot.
     * @param pipeline_id Binding point index.
     */
    virtual std::unique_ptr<ShaderStorageBuffer> createShaderStorageBuffer(u32 pipeline_id) = 0;

    /**
     * @brief Create a 2D texture from a file.
     * @param path Path to the image file.
     */
    virtual std::unique_ptr<Texture> createTexture(const std::string& path) = 0;

    /**
     * @brief Create a cubemap texture from six face images.
     * @param path Array of six file paths in (+X, -X, +Y, -Y, +Z, -Z) order.
     */
    virtual std::unique_ptr<Texture> createCubemap(const std::array<std::string, 6>& path) = 0;

    /**
     * @brief Set the colour used to clear the colour buffer.
     * @param r Red component [0, 1].
     * @param g Green component [0, 1].
     * @param b Blue component [0, 1].
     * @param a Alpha component [0, 1].
     */
    virtual void setClearColor(float r, float g, float b, float a) = 0;

    /// Clear all active framebuffer attachments.
    virtual void clearBuffers() = 0;

    /// Release all device-level resources.
    virtual void cleanup() = 0;

    /**
     * @brief Draw a mesh using its VAO and index buffer.
     * @param mesh Mesh to draw.
     */
    virtual void draw(Mesh& mesh) = 0;

    /**
     * @brief Draw a number of vertices using glDrawArrays.
     * @param indices Number of vertices to draw.
     */
    virtual void draw(u32 indices) = 0;

    /**
     * @brief Draw indexed primitives.
     * @param indices Number of indices.
     */
    virtual void drawIndexed(u32 indices) = 0;

    /// Enable depth testing.
    virtual void enableDepthTest() = 0;

    /// Disable depth testing.
    virtual void disableDepthTest() = 0;

    /// Enable back-face culling.
    virtual void enableFaceCulling() = 0;

    /// Disable back-face culling.
    virtual void disableFaceCulling() = 0;

    /**
     * @brief Draw a mesh with hardware instancing.
     * @param mesh          Mesh to draw.
     * @param instanceCount Number of instances.
     */
    virtual void drawInstanced(Mesh& mesh, u32 instanceCount) = 0;
  };

} // namespace mam
