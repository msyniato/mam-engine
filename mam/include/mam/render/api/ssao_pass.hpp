#pragma once

#include "common/common.hpp"
#include "render/api/render_pass.hpp"
#include "render/api/frame_buffer.hpp"
#include "render/api/texture.hpp"

namespace mam {

  class GraphicsDevice;
  class Pipeline;
  class VertexArray;
  class VertexBuffer;

  /**
   * @brief Screen-Space Ambient Occlusion (SSAO) pass for the deferred pipeline.
   *
   * Reads the G-Buffer produced by GeometryPass (world-space positions and
   * normals) and outputs a blurred per-pixel ambient occlusion factor that
   * LightingPass uses to darken occluded surfaces.
   *
   * Algorithm overview:
   *   1. A 64-sample hemisphere kernel is generated at init time with samples
   *      accelerated toward the origin (more samples near the surface).
   *   2. A 4x4 tileable noise texture supplies a random per-pixel rotation
   *      to the kernel, hiding the regular sampling pattern.
   *   3. Each sample is projected into screen space; the G-Buffer position at
   *      that UV is compared in view-space depth to determine occlusion.
   *   4. A 4x4 box-blur pass removes the noise while preserving low-frequency
   *      occlusion.
   *
   * Toggle: set RenderPass::enabled = false to skip both sub-passes and
   * let LightingPass fall back to full (unoccluded) ambient lighting.
   *
   * Performance: GPU pass duration is measured each frame via a
   * GL_TIME_ELAPSED query and exposed through getLastPassMs().
   */
  class SSAOPass : public RenderPass {
  public:
    /**
     * @brief Construct the pass bound to a graphics device.
     * @param gd Graphics device used for resource creation and draw calls.
     */
    explicit SSAOPass(GraphicsDevice& gd) : gd_(gd) {}
    ~SSAOPass() override;

    /**
     * @brief Initialise framebuffers, pipelines, kernel, and noise texture.
     * @param width  Viewport width in pixels.
     * @param height Viewport height in pixels.
     */
    void init(u32 width, u32 height) override;

    /**
     * @brief Execute the SSAO pass + blur pass for the current frame.
     * @param ctx Frame context; provides camera matrices from RenderPassContext.
     */
    void execute(const RenderPassContext& ctx) override;

    /**
     * @brief Recreate resolution-dependent resources after a viewport resize.
     * @param width  New viewport width in pixels.
     * @param height New viewport height in pixels.
     */
    void resize(u32 width, u32 height) override;

    /// Returns the blurred AO map (consumed by LightingPass as u_ssaoMap).
    FrameBuffer* getOutput() const override { return blurOutput_.get(); }

    /// Returns the raw (pre-blur) SSAO framebuffer (useful for debugging).
    FrameBuffer* getRawOutput() const { return rawOutput_.get(); }

    /**
     * @brief Provide the G-Buffer written by GeometryPass.
     * @param gb Pointer to the G-Buffer framebuffer (not owned by this pass).
     */
    void setGBuffer(FrameBuffer* gb) { gBuffer_ = gb; }

    /**
     * @brief GPU time (milliseconds) spent on this pass in the previous frame.
     *
     * Updated every frame via a non-blocking GL_TIME_ELAPSED query.
     * Returns 0 until the first valid measurement is available.
     */
    float getLastPassMs() const { return lastPassMs_; }

    /// Number of hemisphere samples in the SSAO kernel.
    static constexpr int KERNEL_SIZE = 64;

  private:
    void generateSamples();       ///< Build the 64-sample hemisphere kernel.
    void generateNoiseTexture();  ///< Create the 4x4 RG32F noise texture.
    void initQuad();              ///< Initialise the full-screen quad geometry.
    void drawQuad();              ///< Render the full-screen quad.

    std::unique_ptr<FrameBuffer>  rawOutput_;     ///< Raw SSAO result (RGBA8).
    std::unique_ptr<FrameBuffer>  blurOutput_;    ///< Blurred AO output (RGBA8).
    std::unique_ptr<Pipeline>     ssaoPipeline_;  ///< SSAO shader pipeline.
    std::unique_ptr<Pipeline>     blurPipeline_;  ///< Box-blur shader pipeline.
    std::unique_ptr<VertexArray>  quadVAO_;       ///< Full-screen quad VAO.
    std::unique_ptr<VertexBuffer> quadVBO_;       ///< Full-screen quad VBO.
    std::unique_ptr<Texture>      noiseTexture_;  ///< 4x4 RG32F noise texture.
    FrameBuffer*                  gBuffer_ = nullptr; ///< Input G-Buffer (not owned).
    GraphicsDevice&               gd_;           ///< Graphics device reference.

    std::vector<glm::vec3> samples_;    ///< 64-sample hemisphere kernel.
    u32   width_       = 0;
    u32   height_      = 0;
    u32   queryId_     = 0;            ///< GL_TIME_ELAPSED timer query object.
    float lastPassMs_  = 0.f;          ///< Last measured GPU pass duration (ms).
  };

} // namespace mam
