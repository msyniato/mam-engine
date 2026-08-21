#pragma once

#include "render/api/render_pass.hpp"
#include "render/api/frame_buffer.hpp"

namespace mam {

  class Pipeline;
  class GraphicsDevice;

  /**
   * @brief Shadow-map pass that renders the scene depth from the light's point of view.
   *
   * Produces a depth-only framebuffer at a fixed resolution (SHADOW_W × SHADOW_H)
   * and computes the light-space matrix used by subsequent passes to perform
   * shadow comparisons.
   */
  class ShadowPass : public RenderPass {
  public:
    /**
     * @brief Construct the pass bound to a graphics device.
     * @param gd Graphics device used for draw calls and resource creation.
     */
    ShadowPass(GraphicsDevice& gd) : gd_(gd) {}
    ~ShadowPass() = default;

    /**
     * @brief Initialise the depth framebuffer. The shadow map is always
     *        SHADOW_W × SHADOW_H regardless of the viewport size parameters.
     * @param width  Viewport width in pixels (unused for shadow map size).
     * @param height Viewport height in pixels (unused for shadow map size).
     */
    void init(u32 width, u32 height)           override;

    /**
     * @brief Render the scene from the primary light's perspective.
     * @param ctx Frame context with render queue, camera, and lights.
     */
    void execute(const RenderPassContext& ctx) override;

    /**
     * @brief Handle a viewport resize (shadow map resolution is fixed).
     * @param width  New viewport width in pixels.
     * @param height New viewport height in pixels.
     */
    void resize(u32 width, u32 height)         override;

    /// Returns the depth framebuffer written by this pass.
    FrameBuffer* getOutput() const override { return framebuffer_.get(); }

    /**
     * @brief Return the light-space (view-projection) matrix computed during execute().
     * @return Const reference to the light-space matrix.
     */
    const glm::mat4& getLightSpaceMatrix() const { return lightSpaceMatrix_; }

    /**
     * @brief Set the orthographic frustum half-extent used when building the
     *        light-space matrix.  Smaller values = more texels per world unit =
     *        sharper shadows on small scenes.  Default is 512 (terrain scale).
     * @param halfExtent Half-width/-height of the ortho frustum in world units.
     */
    void setOrthoExtent(float halfExtent) { orthoExtent_ = halfExtent; }

    /// Returns the current ortho half-extent.
    float getOrthoExtent() const { return orthoExtent_; }

  private:
    std::unique_ptr<FrameBuffer> framebuffer_;           ///< Depth-only shadow-map framebuffer.
    std::unique_ptr<Pipeline>    pipeline_;              ///< Depth-only shadow pipeline.
    GraphicsDevice&              gd_;                   ///< Graphics device reference.
    glm::mat4                    lightSpaceMatrix_{1.f}; ///< Light-space VP matrix from the last execute().
    float                        orthoExtent_ = 512.f;  ///< Ortho frustum half-extent (world units).

    static constexpr u32 SHADOW_W = 2048; ///< Fixed shadow-map width in texels.
    static constexpr u32 SHADOW_H = 2048; ///< Fixed shadow-map height in texels.
  };

} // namespace mam
