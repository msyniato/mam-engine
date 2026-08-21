#pragma once

#include "render/api/render_pass.hpp"
#include "render/api/frame_buffer.hpp"

namespace mam {

  class GraphicsDevice;
  class Pipeline;
  class VertexArray;
  class VertexBuffer;
  class ShaderStorageBuffer;

  /**
   * @brief Deferred lighting pass that resolves the G-Buffer into a lit HDR image.
   *
   * Reads the G-Buffer produced by GeometryPass, uploads scene lights to an SSBO,
   * and renders a full-screen quad to compute the final lit colour. The result is
   * written to an owned HDR output framebuffer consumed by subsequent passes.
   */
  class LightingPass : public RenderPass {
  public:
    /**
     * @brief Construct the pass bound to a graphics device.
     * @param gd Graphics device used for draw calls and resource creation.
     */
    LightingPass(GraphicsDevice& gd) : gd_(gd) {}

    /**
     * @brief Initialise the output framebuffer, light SSBO, and full-screen quad.
     * @param width  Viewport width in pixels.
     * @param height Viewport height in pixels.
     */
    void init(u32 width, u32 height)           override;

    /**
     * @brief Upload lights and resolve the G-Buffer into the output framebuffer.
     * @param ctx Frame context with render queue, camera, and lights.
     */
    void execute(const RenderPassContext& ctx) override;

    /**
     * @brief Recreate the output framebuffer at the new viewport size.
     * @param width  New viewport width in pixels.
     * @param height New viewport height in pixels.
     */
    void resize(u32 width, u32 height)         override;

    /// Returns the lit HDR output framebuffer written by this pass.
    FrameBuffer* getOutput() const override { return output_.get(); }

    /**
     * @brief Provide the G-Buffer framebuffer to read from during execute().
     * @param gb Pointer to the G-Buffer produced by GeometryPass.
     */
    void setGBuffer(FrameBuffer* gb) { gBuffer_ = gb; }

    /**
     * @brief Provide the blurred AO map produced by SSAOPass.
     *
     * When set and ssaoEnabled is true, the ambient term in the lighting
     * shader is multiplied by the AO factor read from this texture.
     * @param ssaoFB Pointer to SSAOPass::getOutput() (not owned here).
     */
    void setSSAOMap(FrameBuffer* ssaoFB) { ssaoBuffer_ = ssaoFB; }

    /**
     * @brief Provide the depth framebuffer produced by ShadowPass.
     *
     * When set, the lighting shader samples the shadow map and attenuates
     * directional light contributions by the PCF shadow factor.
     * @param shadowFB Pointer to ShadowPass::getOutput() (not owned here).
     */
    void setShadowMap(FrameBuffer* shadowFB) { shadowBuffer_ = shadowFB; }

    /// Enable or disable SSAO contribution in the lighting shader.
    bool ssaoEnabled = false;

  private:
    void initQuad(); ///< Initialise the full-screen quad geometry.
    void drawQuad(); ///< Render the full-screen quad to trigger the lighting shader.

    std::unique_ptr<FrameBuffer>         output_;      ///< Lit HDR output framebuffer.
    std::unique_ptr<ShaderStorageBuffer> lightBuffer_; ///< GPU light data SSBO.
    std::unique_ptr<Pipeline>            pipeline_;    ///< Deferred lighting shader pipeline.
    std::unique_ptr<VertexArray>         quadVAO_;     ///< Full-screen quad VAO.
    std::unique_ptr<VertexBuffer>        quadVBO_;     ///< Full-screen quad VBO.
    FrameBuffer*                         gBuffer_      = nullptr; ///< Input G-Buffer (owned by GeometryPass).
    FrameBuffer*                         ssaoBuffer_   = nullptr; ///< Blurred AO map (owned by SSAOPass).
    FrameBuffer*                         shadowBuffer_ = nullptr; ///< Depth shadow map (owned by ShadowPass).
    GraphicsDevice&                      gd_;          ///< Graphics device reference.
  };

} // namespace mam
