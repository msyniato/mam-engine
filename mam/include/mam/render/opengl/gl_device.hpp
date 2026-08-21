#pragma once

#include "render/api/graphics_device.hpp"

namespace mam {

  class GLWindow;

  /**
   * @brief OpenGL implementation of GraphicsDevice.
   *
   * Creates and manages all OpenGL GPU resources and issues draw calls
   * through the OpenGL 4.5 API.
   */
  class GLDevice : public GraphicsDevice
  {
  public:
    GLDevice() = default;
    ~GLDevice() override = default;

    std::unique_ptr<Shader>   createShader()   override;
    std::unique_ptr<Pipeline> createPipeline() override;

    std::unique_ptr<VertexBuffer> createVertexBuffer(unsigned int count = 0)               override;
    std::unique_ptr<VertexBuffer> createVertexBuffer(void* vertex, unsigned int count)     override;

    std::unique_ptr<IndexBuffer>  createIndexBuffer(unsigned int size = 0)                 override;
    std::unique_ptr<IndexBuffer>  createIndexBuffer(void* index, unsigned int size)        override;

    std::unique_ptr<FrameBuffer>  createFrameBuffer(const FrameBufferSpec& spec)           override;
    std::unique_ptr<VertexArray>  createVertexArray()                                      override;
    std::unique_ptr<GBuffer>      createGBuffer(glm::vec2 screen_size)                    override;
    std::unique_ptr<ShaderStorageBuffer> createShaderStorageBuffer(u32 pipeline_id)       override;

    std::unique_ptr<Texture> createTexture(const std::string& path)                       override;
    std::unique_ptr<Texture> createCubemap(const std::array<std::string, 6>& path)        override;

    void setClearColor(float r, float g, float b, float a) override;
    void clearBuffers() override;
    void cleanup()      override;

    void draw(Mesh& mesh)     override;
    void draw(u32 indices)    override;
    void drawIndexed(u32 indices) override;

    void enableDepthTest()    override;
    void disableDepthTest()   override;
    void enableFaceCulling()  override;
    void disableFaceCulling() override;

    void drawInstanced(Mesh& mesh, u32 instanceCount) override;
  };

} // namespace mam
