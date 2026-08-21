#pragma once

#include "render/api/render_pass.hpp"
#include "render/api/frame_buffer.hpp"

namespace mam {

  class GraphicsDevice;
  class Pipeline;

  /**
   * @brief Deferred geometry pass that fills the G-Buffer with surface attributes.
   *
   * Renders all opaque meshes from the RenderPassContext into a multi-target
   * G-Buffer (position, normal, albedo, etc.). The output is consumed by the
   * LightingPass in a subsequent step of the deferred pipeline.
   */
  class GeometryPass : public RenderPass {
  public:
    /**
     * @brief Construct the pass bound to a graphics device.
     * @param gd Graphics device used for draw calls and resource creation.
     */
    GeometryPass(GraphicsDevice& gd) : gd_(gd) {}

    /**
     * @brief Initialise the G-Buffer and depth attachments at the given viewport size.
     * @param width  Viewport width in pixels.
     * @param height Viewport height in pixels.
     */
    void init(u32 width, u32 height)           override;

    /**
     * @brief Draw all opaque geometry into the G-Buffer.
     * @param ctx Frame context with render queue, camera, and lights.
     */
    void execute(const RenderPassContext& ctx) override;

    /**
     * @brief Recreate the G-Buffer at the new viewport size.
     * @param width  New viewport width in pixels.
     * @param height New viewport height in pixels.
     */
    void resize(u32 width, u32 height)         override;

    /// Returns the G-Buffer framebuffer written by this pass.
    FrameBuffer* getOutput() const override { return gBuffer_.get(); }

  private:
    std::unique_ptr<FrameBuffer> gBuffer_;          ///< Multi-target G-Buffer output.
    std::unique_ptr<Pipeline>    pipeline_;         ///< Generic geometry pipeline.
    std::unique_ptr<Pipeline>    terrainPipeline_;  ///< Terrain splat-blending pipeline.
    GraphicsDevice&              gd_;               ///< Graphics device reference.
  };

} // namespace mam
