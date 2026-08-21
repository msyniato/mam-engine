#pragma once

#include "common/common.hpp"
#include "render/api/render_graph.hpp"

namespace mam {

  class ShaderStorageBuffer;
  class GraphicsDevice;
  class VertexBuffer;
  class VertexArray;
  class FrameBuffer;
  class ForwardPass;
  class Pipeline;
  class Texture;
  class Material;
  class GBuffer;
  class World;
  class Mesh;
  class Camera;
  class ImGUIWrapper;
  class Context;
  class ShadowPass;
  class GeometryPass;
  class LightingPass;
  class SSAOPass;
  
  enum class RenderLayer : u8 {
    Opaque      = 0,
    ForwardOnly = 1,
    Transparent = 2,
    Overlay     = 3
  };

  /**
   * @brief A single non-instanced draw command submitted to the render queue.
   *
   * Carries everything a render pass needs to issue one draw call:
   * material, mesh, world transform, texture bindings, and render layer.
   */
  struct RenderCommand {
    Material*             material;                        ///< Material to use for this draw call.
    Mesh*                 mesh;                            ///< Mesh geometry to draw.
    glm::mat4             transform;                       ///< World-space model transform.
    std::vector<Texture*> textures;                        ///< Ordered texture bindings.
    RenderLayer           layer        = RenderLayer::Opaque; ///< Render layer for sorting.
    bool                  useSplatting = false;            ///< True for terrain chunks using splat-blended textures.
  };

  /**
   * @brief A single instanced draw command submitted to the render queue.
   *
   * Extends RenderCommand with a GPU instance buffer and an instance count
   * so that many copies of the same mesh are drawn in a single call.
   */
  struct RenderCommandInstanced {
    Material*             material;                        ///< Material to use for this draw call.
    Mesh*                 mesh;                            ///< Mesh geometry to draw.
    VertexBuffer*         instanceBuffer;                  ///< Per-instance attribute buffer.
    u32                   instanceCount;                   ///< Number of instances to draw.
    std::vector<Texture*> textures;                        ///< Ordered texture bindings.
    RenderLayer           layer = RenderLayer::Opaque;     ///< Render layer for sorting.
  };

  /**
   * @brief Combines multiple static meshes into a single batched draw call.
   *
   * Entries are accumulated with add(), then build() merges them into one Mesh
   * object. After building, getBatchedMesh() returns the combined geometry.
   */
  class StaticBatcher {
  public:
    /**
     * @brief Add a mesh to the pending batch.
     * @param mesh      Source mesh to include.
     * @param transform World-space transform applied to this mesh's vertices.
     */
    void add(Mesh* mesh, const glm::mat4& transform);

    /**
     * @brief Merge all pending entries into a single GPU-ready mesh.
     * @param gd   Graphics device used to upload the combined buffers.
     * @param name Debug name for the batched mesh (defaults to "batch").
     */
    void build(GraphicsDevice& gd, const std::string& name = "batch");

    /// Clear all pending entries and release the batched mesh.
    void clear();

    /// Returns true after build() has been called successfully.
    bool  isBuilt()        const { return built_; }

    /// Returns the combined mesh produced by build(), or nullptr if not yet built.
    Mesh* getBatchedMesh() const { return batchedMesh_.get(); }

  private:
    /// Internal entry storing a mesh and its transform.
    struct Entry { Mesh* mesh; glm::mat4 transform; };

    std::vector<Entry>    entries_;      ///< Pending entries to be merged.
    std::unique_ptr<Mesh> batchedMesh_;  ///< Combined mesh produced by build().
    bool                  built_;        ///< Whether build() has completed successfully.
  };

  /**
   * @brief Accumulates render commands for a single frame.
   *
   * Systems call submit() or submitInstanced() to enqueue draw commands.
   * The active render passes then drain the queue during their execute() calls.
   */
  class RenderQueue {
  public:
    RenderQueue()  = default;
    ~RenderQueue() = default;

    /**
     * @brief Enqueue a non-instanced draw command.
     * @param cmd Command to submit.
     */
    void submit(const RenderCommand& cmd);

    /**
     * @brief Enqueue an instanced draw command.
     * @param cmd Instanced command to submit.
     */
    void submitInstanced(const RenderCommandInstanced& cmd);

    /**
     * @brief Return all non-instanced commands submitted this frame.
     * @return Mutable reference to the command list.
     */
    std::vector<RenderCommand>& getRenderCommands();

    /**
     * @brief Return all instanced commands submitted this frame.
     * @return Mutable reference to the instanced command list.
     */
    std::vector<RenderCommandInstanced>& getInstancedCommands();

    /// Clear all commands, typically called at the start of each frame.
    void clear();

  private:
    std::vector<RenderCommand>          renderCommands_;    ///< Non-instanced draw commands.
    std::vector<RenderCommandInstanced> instancedCommands_; ///< Instanced draw commands.
  };

  /**
   * @brief High-level renderer that owns the render graph and orchestrates frame rendering.
   *
   * Constructs the render graph (currently a single ForwardPass), provides
   * resize handling, skybox configuration, and both standard and Vulkan-ImGui
   * render entry points.
   */
  class Renderer {
  public:
    /**
     * @brief Construct the renderer for the given device and initial viewport size.
     * @param gd          Graphics device used by all render passes.
     * @param screen_size Initial viewport dimensions in pixels.
     */
    Renderer(GraphicsDevice& gd, const glm::vec2& screen_size);
    ~Renderer();

    /**
     * @brief Render the queued commands for the current frame (OpenGL path).
     * @param renderQueue Commands to draw.
     * @param camera      Active scene camera.
     */
    void render(RenderQueue& renderQueue, Camera& camera);

    /**
     * @brief Render the queued commands for the current frame (Vulkan + ImGui path).
     * @param renderQueue Commands to draw.
     * @param camera      Active scene camera.
     * @param imgui       ImGui wrapper used by the ForwardPass (Vulkan only).
     * @param appContext  Application ECS context forwarded to the pass.
     */
    void render(RenderQueue& renderQueue,
                Camera& camera,
                ImGUIWrapper* imgui,
                Context* appContext);

    /**
     * @brief Notify the renderer of a viewport resize.
     * @param width  New viewport width in pixels.
     * @param height New viewport height in pixels.
     */
    void resize(u32 width, u32 height);

    /**
     * @brief Return the output framebuffer of the forward pass.
     * @return Pointer to the final rendered framebuffer.
     */
    FrameBuffer* getFramebuffer() const;

    /// Returns a mutable reference to the render graph.
    RenderGraph& getRenderGraph() { return renderGraph_; }

    /// Returns a mutable reference to the scene light list.
    LightList& getLightsList() { return lightList_; }

    /**
     * @brief Configure the skybox rendered after opaque geometry.
     * @param cubemap Cubemap texture used for the skybox.
     * @param mat     Skybox material.
     */
    void setSkybox(Texture* cubemap, Material* mat);

    /**
     * @brief Enable or disable the SSAO pass at runtime.
     *
     * When enabled the SSAOPass executes between GeometryPass and LightingPass
     * and the ambient term in the lighting shader is attenuated by the AO map.
     * When disabled both sub-passes are skipped and ambient is unaffected.
     * @param on True to enable SSAO, false to disable.
     */
    void setSSAOEnabled(bool on);

    /**
     * @brief Returns the GPU time (ms) spent on the SSAO pass last frame.
     * @return 0 if SSAO is disabled or no measurement is available yet.
     */
    float getSSAOPassMs() const;

    /**
     * @brief Set the ortho half-extent of the shadow map frustum.
     *
     * Smaller values concentrate shadow map texels over a tighter area,
     * improving quality for small scenes.  Default is 512 (terrain scale).
     * Call this before the first frame — the change takes effect immediately.
     * @param halfExtent Half-width/-height of the ortho box in world units.
     */
    void setShadowOrthoExtent(float halfExtent);

    void setDeferredEnabled(bool on);

    bool isDeferred_ = true;
  private:
    
    GraphicsDevice& graphicsDevice_;
    LightList       lightList_;
    ForwardPass*    forwardPass_  = nullptr;
    GeometryPass*   geometryPass_ = nullptr;
    SSAOPass*       ssaoPass_     = nullptr;
    LightingPass*   lightingPass_ = nullptr;
    ShadowPass*     shadowPass_   = nullptr;
    RenderGraph     renderGraph_;
    bool ssaoEnabled_ = false;
    // Can be used for drawing the scene directly into window
    std::unique_ptr<Pipeline> renderPipeline_;
    std::unique_ptr<VertexArray> quadVAO_;
    std::unique_ptr<VertexBuffer> quadVBO_;
		std::unique_ptr<Mesh> skyboxCube_;
    void initQuad();
    void drawQuad();
    
  };

} // namespace mam
