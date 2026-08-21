#pragma once

#include "render/api/pipeline.hpp"
#include <vulkan/vulkan.hpp>

namespace mam {

  class Shader;
  class VKShader;
  class VKDevice;
  class VKTexture;

  /**
   * @brief Descriptor structure used to configure a Vulkan graphics pipeline.
   *
   * Groups all Vulkan pipeline state objects required during pipeline creation.
   * Acts as an intermediate container that accumulates vertex-input, rasterization,
   * viewport, blending, and shader-stage configuration before the final
   * vkCreateGraphicsPipelines() call.
   */
  struct VKPipelineDesc {
    std::vector<vk::VertexInputAttributeDescription> inputAttributes; ///< Per-attribute location, binding, format, and offset.
    vk::VertexInputBindingDescription                inputBinding;    ///< Vertex buffer binding (stride, input rate).
    vk::PipelineVertexInputStateCreateInfo           vertexInputInfo; ///< Aggregates inputAttributes + inputBinding.
    vk::PipelineInputAssemblyStateCreateInfo         inputAssembly;   ///< Primitive topology (triangles, lines, etc.).
    vk::Viewport                                     viewport;        ///< Viewport transform (x, y, w, h, minDepth, maxDepth).
    vk::Rect2D                                       scissor;         ///< Scissor rectangle for fragment clipping.
    vk::PipelineViewportStateCreateInfo              viewportState;   ///< Aggregates viewport + scissor.
    vk::PipelineRasterizationStateCreateInfo         rasterizer;      ///< Fill mode, culling, front-face winding, depth bias.
    vk::PipelineMultisampleStateCreateInfo           multisampling;   ///< MSAA sample count and settings.
    vk::PipelineColorBlendAttachmentState            colorBlendAttachment;  ///< Blending for a single attachment (legacy single-output).
    std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments; ///< Blending per attachment (MRT support).
    vk::PipelineColorBlendStateCreateInfo            colorBlending;   ///< Global blending / logic-op settings.
    vk::PipelineShaderStageCreateInfo                shaderStages[2]; ///< Vertex [0] and fragment [1] stage descriptors.
    vk::ShaderModule                                 shaderModule[2]; ///< Compiled SPIR-V modules for vertex [0] and fragment [1].
    std::vector<uint32_t>                            spirvCache_[2];  ///< Cached SPIR-V words for hot-reload / pipeline recreation.
  };

  /**
   * @brief Uniform buffer layout shared by all standard PBR shaders.
   *
   * Contains the MVP matrices, camera position, light count, and
   * all Disney-principled BRDF material parameters. Must match the
   * std140 layout declared in the corresponding GLSL/SPIR-V shaders.
   */
  struct UniformBufferObject {
    glm::mat4 model; ///< Model-to-world transformation matrix.
    glm::mat4 view;  ///< World-to-camera (view) matrix.
    glm::mat4 proj;  ///< Camera-to-clip (projection) matrix.

    glm::vec3 viewPos;    float _pad0;          ///< Camera world position (w padded for std140 alignment).
    int       lightCount; float _pad1, _pad2, _pad3; ///< Number of active lights (xyz padded for std140 alignment).

    // Disney principled BRDF material parameters
    glm::vec3 baseColor;  float metallic;        ///< Base albedo color + metallic factor [0, 1].
    float     roughness;  float specular;        ///< Surface roughness [0, 1] + specular intensity [0, 1].
    float     specularTint; float anisotropic;   ///< Specular tint [0, 1] + anisotropy [-1, 1].
    float     sheen;      float sheenTint;       ///< Sheen intensity [0, 1] + sheen tint [0, 1].
    float     clearcoat;  float clearcoatGloss;  ///< Clear-coat intensity [0, 1] + gloss [0, 1].

    int useAlbedoMap;    ///< Non-zero if an albedo texture is bound.
    int useNormalMap;    ///< Non-zero if a normal map is bound.
    int useRoughnessMap; ///< Non-zero if a roughness map is bound.
    int useMetallicMap;  ///< Non-zero if a metallic map is bound.
  };

  /**
   * @brief Metadata describing the location of a uniform within the CPU-side buffer.
   */
  struct VKUniformInfo {
    uint32_t offset; ///< Byte offset of this uniform from the start of the uniform buffer.
    uint32_t size;   ///< Size of this uniform in bytes.
  };

  /**
   * @brief Per-material descriptor set and its associated GPU/CPU uniform data.
   *
   * Each unique combination of textures used by a draw call gets its own
   * VKMaterialDescriptor so that texture bindings and per-material uniforms
   * can vary independently without stalling the GPU.
   */
  struct VKMaterialDescriptor {
    vk::DescriptorSet  descriptorSet = nullptr; ///< Descriptor set (set 1) bound for this material.
    vk::Buffer         uniformBuffer = nullptr; ///< GPU uniform buffer for per-material parameters.
    vk::DeviceMemory   uniformMemory = nullptr; ///< Device memory backing uniformBuffer.
    std::vector<u8>    cpuData;                  ///< CPU-side copy of the uniform data (for updates).
  };

  /**
   * @brief Vulkan implementation of the Pipeline interface.
   *
   * VKPipeline manages the full lifecycle of a Vulkan graphics pipeline:
   *
   * - **Shader attachment** — accepts compiled SPIR-V via addShader().
   * - **State configuration** — vertex input, input assembly, viewport, rasterizer, blending.
   * - **Descriptor infrastructure** — layout, pool, and per-frame descriptor sets.
   * - **Uniform management** — named uniform registry, CPU-side buffer, and GPU upload.
   * - **Texture binding** — per-material descriptor sets keyed on texture combinations.
   * - **Pipeline recreation** — triggered by the device on swapchain resize.
   *
   * @note Copy and move operations are disabled. Pipelines must be managed
   *       via shared_ptr so the device can hold weak_ptr references for recreation.
   */
  class VKPipeline : public Pipeline {
  public:

    /**
     * @brief Constructs an unconfigured pipeline.
     *
     * @param device  Non-owning pointer to the owning VKDevice. Must remain
     *                valid for the entire lifetime of this pipeline.
     */
    VKPipeline(VKDevice* device);

    /**
     * @brief Destructor. Calls destroy() to release all Vulkan objects.
     */
    ~VKPipeline();

    VKPipeline(const VKPipeline&) = delete;
    VKPipeline& operator=(const VKPipeline&) = delete;
    VKPipeline(VKPipeline&&) noexcept = delete;
    VKPipeline& operator=(VKPipeline&&) noexcept = delete;

    /**
     * @brief Attaches a shader stage to this pipeline.
     *
     * The shader's stage type (vertex / fragment) is inferred from the
     * VKShader object. Must be called before create().
     *
     * @param shader  Non-owning pointer to a compiled shader. The shader must
     *                outlive the create() call.
     */
    virtual void addShader(Shader* shader) override;

    /**
     * @brief Compiles all configured state into the final VkPipeline object.
     *
     * Calls initStructs(), createDescriptorSetLayout(), createDescriptorPool(),
     * createUniformBuffer(), createDescriptorSets(), and vkCreateGraphicsPipelines().
     *
     * @return True if pipeline creation succeeded, false otherwise.
     */
    virtual std::optional<std::string> create() override;

    /**
     * @brief Recreates the pipeline after a swapchain resize.
     *
     * Destroys the existing pipeline and rebuilds it with the updated
     * swapchain extent and render pass.
     */
    void recreatePipeline();

    /**
     * @brief Binds this pipeline for subsequent draw calls.
     *
     * Records vkCmdBindPipeline and uploads the current uniform buffer
     * contents to the GPU.
     */
    virtual void use() override;

    /**
     * @brief Releases all Vulkan objects owned by this pipeline.
     *
     * Destroys the VkPipeline, VkPipelineLayout, descriptor pool/layout,
     * uniform buffer, and shader modules.
     */
    void destroy();

    /**
     * @brief Initializes all VKPipelineDesc state-create-info structures to safe defaults.
     *
     * Must be called before any individual setXxx() configuration methods.
     */
    void initStructs();

    /**
     * @brief Returns the byte offset of a named uniform within the uniform buffer.
     *
     * @param name  Name of the uniform as declared in the shader.
     * @return      Byte offset, or -1 if the uniform was not found.
     */
    virtual int getUniformLocation(const char* name) const override;

    /**
     * @brief Writes a value into the CPU-side uniform buffer.
     *
     * The data will be uploaded to the GPU on the next use() call.
     *
     * @param name   Name of the uniform (must have been registered via CreateUniform()).
     * @param value  Pointer to the data to write (must match the registered size).
     */
    virtual void setUniform(const char* name, const void* value) override;

    /**
     * @brief Binds a sampler uniform to a specific texture unit.
     *
     * Updates the descriptor set to associate the named sampler with
     * the given binding index.
     *
     * @param name  Name of the sampler uniform in the shader.
     * @param unit  Texture binding index.
     */
    virtual void setSamplerUniform(const char* name, int unit) override;

    /**
     * @brief Registers a uniform in the pipeline's name-to-offset map.
     *
     * Must be called before create() for each uniform that will be set
     * at runtime via setUniform().
     *
     * @param name    Uniform name (must match the shader variable name).
     * @param offset  Byte offset of the uniform within the uniform buffer.
     * @param size    Size of the uniform in bytes.
     */
    virtual void CreateUniform(std::string name, uint32_t offset, uint32_t size) override;

    /**
     * @brief Binds a texture to a descriptor set binding.
     *
     * @param tex      Texture to bind.
     * @param set      Descriptor set index (default 0).
     * @param binding  Binding index within the descriptor set (default 0).
     */
    virtual void bindTexture(const Texture* tex, u32 set = 0, u32 binding = 0) override;

    /**
     * @brief Returns the internal pipeline identifier.
     * @return Unique pipeline ID.
     */
    virtual u32 id() const override;

    /**
     * @brief Adds a vertex attribute description.
     *
     * @param binding   Vertex buffer binding index.
     * @param location  Shader input location.
     * @param format    Vulkan format of the attribute (e.g. eR32G32B32Sfloat for vec3).
     * @param offset    Byte offset of this attribute within the vertex struct.
     */
    void addAttribute(u32 binding, u32 location, vk::Format format, u32 offset);

    /**
     * @brief Configures the vertex buffer binding description.
     *
     * @param binding    Binding index.
     * @param stride     Size of one vertex in bytes.
     * @param inputRate  eVertex (per-vertex) or eInstance (per-instance).
     */
    void setBindingDescription(u32 binding, u32 stride, vk::VertexInputRate inputRate);

    /**
     * @brief Sets the input assembly primitive topology.
     *
     * @param drawMode  Topology to use (default: eTriangleList).
     */
    void setInputAssembly(vk::PrimitiveTopology drawMode = vk::PrimitiveTopology::eTriangleList);

    /**
     * @brief Configures the viewport transform.
     *
     * @param width   Viewport width in pixels.
     * @param height  Viewport height in pixels.
     * @param depth   Far-plane depth value (default 1.0).
     */
    void setViewport(float width, float height, float depth = 1.0f);

    /**
     * @brief Configures the scissor rectangle.
     *
     * @param offsetX  Horizontal pixel offset (default 0).
     * @param offsetY  Vertical pixel offset (default 0).
     */
    void setScissor(s32 offsetX = 0, s32 offsetY = 0);

    /**
     * @brief Configures rasterization state.
     *
     * @param depthClampEnable  Clamp fragments outside [0,1] depth instead of discarding.
     * @param DiscardEnable     Discard all primitives before rasterization (geometry-only pass).
     * @param mode              Polygon fill mode (eFill, eLine, ePoint).
     * @param cullmode          Face culling mode (eNone, eFront, eBack, eFrontAndBack).
     * @param frontface         Front-face winding order (eClockwise or eCounterClockwise).
     * @param depthBiasEnable   Enable depth bias (useful for shadow mapping).
     */
    void setRasterizer(bool depthClampEnable = false, bool DiscardEnable = false,
      vk::PolygonMode mode = vk::PolygonMode::eFill,
      vk::CullModeFlagBits cullmode = vk::CullModeFlagBits::eBack,
      vk::FrontFace frontface = vk::FrontFace::eClockwise,
      bool depthBiasEnable = false);

    /**
     * @brief Configures color-blend state for a single attachment (single render-target path).
     *
     * @param blendEnable  True to enable alpha blending; false for opaque rendering.
     */
    void setColorBlendAttachment(bool blendEnable = false);

    /**
     * @brief Configures global color-blend state.
     *
     * @param logicOpEnable  True to enable logical color operations.
     * @param logicOp        Logical operation to apply when logicOpEnable is true.
     */
    void setColorBlending(bool logicOpEnable = false,
      vk::LogicOp logicOp = vk::LogicOp::eCopy);

    /**
     * @brief Configures blending for multiple render targets (MRT).
     *
     * Creates @p count identical blend-attachment states and updates the
     * color-blend create-info to reference them. Call after setColorBlending().
     *
     * @param count        Number of color attachments.
     * @param blendEnable  True to enable alpha blending on all attachments.
     */
    void setColorAttachmentCount(u32 count, bool blendEnable = false);

    /**
     * @brief Creates the VkDescriptorSetLayout for set 0 (UBO) and set 1 (textures).
     */
    void createDescriptorSetLayout();

    /**
     * @brief Creates the VkDescriptorPool with enough capacity for UBO and texture descriptors.
     */
    void createDescriptorPool();

    /**
     * @brief Allocates and initializes descriptor sets from the pool.
     *
     * Writes the UBO binding and any pre-bound texture bindings.
     */
    void createDescriptorSets();

    /**
     * @brief Binds an SSBO containing light data to the pipeline.
     *
     * Updates the descriptor set to reference the given buffer.
     *
     * @param buffer  GPU buffer holding the packed light data.
     * @param size    Size of the buffer in bytes.
     */
    void bindLightBuffer(vk::Buffer buffer, vk::DeviceSize size);

    /** @brief Maximum number of textures that can be bound simultaneously per material. */
    static constexpr u32 kMaxMaterialTextures = 4;

    /** @brief Key type used to look up or create a per-material descriptor set. */
    using TextureSetKey = std::array<const Texture*, kMaxMaterialTextures>;

    /**
     * @brief Returns (or creates) the descriptor set for the given texture combination.
     *
     * Keyed on the exact set of texture pointers. Two draw calls sharing the same
     * four textures will reuse the same VKMaterialDescriptor.
     *
     * @param textures  Array of up to kMaxMaterialTextures texture pointers.
     *                  Unused slots should be nullptr (resolved to the dummy texture).
     * @return          Non-owning pointer to the cached VKMaterialDescriptor.
     */
    VKMaterialDescriptor* getOrCreateMaterialDescriptorSet(const TextureSetKey& textures);

    /**
     * @brief Binds a pre-built material descriptor set for the next draw call.
     *
     * @param materialDescriptorSet  Descriptor set to bind at set index 1.
     */
    void bindForMaterial(vk::DescriptorSet materialDescriptorSet);

    /**
     * @brief Copies the current CPU uniform data into a material descriptor's GPU buffer.
     *
     * @param descriptor  Material descriptor whose uniformBuffer will be updated.
     */
    void uploadMaterialDescriptorUniforms(VKMaterialDescriptor& descriptor);

    /**
     * @brief Returns a read-only view of the current CPU-side uniform data.
     *
     * Useful for copying uniform state to per-material descriptors without
     * triggering a GPU upload.
     *
     * @return Const reference to the internal CPU uniform byte buffer.
     */
    const std::vector<uint8_t>& cpuUniformData() const { return mappedUniforms_; }

    vk::RenderPass renderPass; ///< Render pass this pipeline is compatible with.

  private:

    /**
     * @brief Allocates and maps the GPU uniform buffer.
     *
     * Size is computed from the registered uniforms; called by create().
     */
    void createUniformBuffer();

    /** @brief Builds the VkPipelineVertexInputStateCreateInfo from registered attributes. */
    void setVertexInputInfo();

    /** @brief Builds the VkPipelineViewportStateCreateInfo from the configured viewport/scissor. */
    void setViewportState();

    /** @brief Sets multisampling to 1 sample (MSAA disabled). */
    void setMultisampling();

    /**
     * @brief Finds a device memory type satisfying the given constraints.
     *
     * @param typeFilter  Bitmask of acceptable memory type indices.
     * @param properties  Required memory property flags.
     * @return            Index of a suitable memory type.
     */
    uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

    /**
     * @brief Creates a 1×1 white dummy texture for unbound sampler slots.
     *
     * Ensures descriptor sets are always fully populated, satisfying the
     * Vulkan validation requirement that all bound samplers reference valid images.
     */
    void createDummyTexture();

    /**
     * @brief Allocates and populates a new VKMaterialDescriptor for the given textures.
     *
     * @param textures  Texture combination to encode into the descriptor set.
     * @return          Newly allocated descriptor with set, buffer, and memory filled in.
     */
    VKMaterialDescriptor allocateMaterialDescriptorSet(const TextureSetKey& textures);

    /**
     * @brief Hash functor for TextureSetKey used in the material descriptor map.
     */
    struct TextureSetKeyHash {
      std::size_t operator()(const TextureSetKey& key) const noexcept {
        std::size_t h = 0;
        for (const Texture* tex : key) {
          std::size_t v = std::hash<const Texture*>{}(tex);
          h ^= v + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
        }
        return h;
      }
    };

    /// Cache of per-material descriptor sets, keyed on texture combination.
    std::unordered_map<TextureSetKey, VKMaterialDescriptor, TextureSetKeyHash> materialDescriptorSets_;

    std::unique_ptr<VKTexture>      dummyTexture_;           ///< 1×1 white fallback texture for unbound samplers.
    std::vector<vk::ShaderModule>   ownedModules_;           ///< Shader modules to destroy on pipeline teardown.
    vk::Buffer                      boundLightBuffer_{ nullptr };      ///< Currently bound light SSBO.
    vk::DeviceSize                  boundLightBufferSize_{ 0 };        ///< Size of the bound light buffer in bytes.
    std::unordered_map<std::string, VKUniformInfo> uniforms; ///< Name -> (offset, size) uniform registry.
    std::vector<uint8_t>            mappedUniforms_;         ///< CPU-side uniform byte buffer.
    vk::DeviceSize                  uniformBufferSize;       ///< Total size of the uniform buffer in bytes.
    vk::DescriptorSetLayout         descriptorSetLayout;     ///< Layout for set 0 (UBO + light buffer).
    vk::DescriptorSetLayout         descriptorSetLayout1 = nullptr; ///< Layout for set 1 (material textures).
    std::vector<vk::DescriptorSet>  descriptorSet;           ///< Allocated descriptor sets for set 0.
    vk::DescriptorPool              descriptorPool;          ///< Pool from which all descriptor sets are allocated.
    vk::Pipeline                    graphicsPipeline;        ///< Compiled Vulkan graphics pipeline.
    vk::PipelineLayout              pipelineLayout;          ///< Pipeline layout (descriptor sets + push constants).
    VKPipelineDesc                  description;             ///< Intermediate description used during pipeline creation.
    VKDevice* vk_device_;              ///< Non-owning pointer to the owning device.
  };

} // namespace mam