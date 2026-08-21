#pragma once

#include "render/api/render_pass.hpp"
#include "render/api/frame_buffer.hpp"

namespace mam {

  class ShaderStorageBuffer;
  class VertexBuffer;
  class VertexArray;
  class Pipeline;
  class Mesh;
  class Material;
  class Texture;

  /**
   * @brief Forward-rendering pass that draws opaque geometry and the skybox.
   *
   * Receives the scene from the RenderPassContext, uploads lights to an SSBO,
   * draws all opaque and instanced meshes, then renders the skybox if one has
   * been set. Output is written to an owned FrameBuffer.
   */
  class ForwardPass : public RenderPass {
  public:
    /**
     * @brief Construct the pass bound to a graphics device.
     * @param gd Graphics device used for draw calls and resource creation.
     */
    ForwardPass(GraphicsDevice& gd) :
      gd_(gd), skyboxMesh_(nullptr),
      skyboxMat_(nullptr), skyboxCubemap_(nullptr) {}

    ~ForwardPass() override = default;

    /**
     * @brief Initialise framebuffer and light SSBO at the given viewport size.
     * @param width  Viewport width in pixels.
     * @param height Viewport height in pixels.
     */
    void init(u32 width, u32 height)           override;

    /**
     * @brief Draw the scene for the current frame.
     * @param ctx Frame context with render queue, camera, and lights.
     */
    void execute(const RenderPassContext& ctx) override;

    /**
     * @brief Recreate the framebuffer at the new viewport size.
     * @param width  New viewport width in pixels.
     * @param height New viewport height in pixels.
     */
    void resize(u32 width, u32 height)         override;

    /**
     * @brief Set the skybox assets rendered after opaque geometry.
     * @param cube     Box mesh used to render the skybox.
     * @param mat      Skybox material.
     * @param cubemap  Cubemap texture.
     */
    void setSkybox(Mesh* cube, Material* mat, Texture* cubemap) {
      skyboxMesh_ = cube; skyboxMat_ = mat; skyboxCubemap_ = cubemap;
    }

    /// En modo deferred, apunta al output de LightingPass para hacer blit de color.
    void setLightingOutput(FrameBuffer* fb) { lightingOutput_ = fb; }

    /// En modo deferred, apunta al G-Buffer para hacer blit de profundidad.
    void setGBuffer(FrameBuffer* gb)        { gBuffer_ = gb; }

    /// Returns the output framebuffer written by this pass.
    FrameBuffer* getOutput() const override { return framebuffer_.get(); }

  private:
    void uploadLights(const RenderPassContext& ctx);
    void drawOpaque(const RenderPassContext& ctx);
    void drawOpaqueInstanced(const RenderPassContext& ctx);
    void drawSkybox(const RenderPassContext& ctx);

    Mesh*     skyboxMesh_;     ///< Skybox box mesh.
    Material* skyboxMat_;      ///< Skybox material.
    Texture*  skyboxCubemap_;  ///< Skybox cubemap texture.

    FrameBuffer* lightingOutput_ = nullptr; ///< LightingPass output (deferred mode blit source).
    FrameBuffer* gBuffer_        = nullptr; ///< G-Buffer depth blit source (deferred mode).

    std::unique_ptr<ShaderStorageBuffer> lightBuffer_;  ///< GPU light data SSBO.
    std::unique_ptr<FrameBuffer>         framebuffer_;  ///< Output framebuffer.
    GraphicsDevice&                      gd_;           ///< Graphics device.
  };

} // namespace mam
