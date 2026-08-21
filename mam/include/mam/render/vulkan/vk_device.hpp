#pragma once

#include "render/api/graphics_device.hpp"
#include "render/api/buffer.hpp"
#include "render/api/vertex_array.hpp"
#include "render/vulkan/vk_pipeline.hpp"
#include <vulkan/vulkan.hpp>
#include <optional>

namespace mam {

  class VKWindow;
  class GraphicContext;
  class VKPipeline;

  /**
   * @brief Holds queue family indices required by the Vulkan device.
   *
   * Vulkan devices expose multiple queue families. This structure stores
   * the indices for the graphics and presentation queues discovered
   * during physical device selection.
   */
  struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily; ///< Queue family supporting graphics commands.
    std::optional<uint32_t> presentFamily;  ///< Queue family supporting presentation to the surface.

    /**
     * @brief Checks whether all required queue families were found.
     * @return True if both graphics and presentation queue indices are present.
     */
    bool isComplete() {
      return graphicsFamily.has_value() && presentFamily.has_value();
    }
  };

  /**
   * @brief Stores swapchain support details for a physical device.
   *
   * Queried once per physical device candidate and used to select
   * the optimal surface format, present mode, and swap extent.
   */
  struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR          capabilities; ///< Surface capabilities (min/max image count, extents, etc.).
    std::vector<vk::SurfaceFormatKHR>   formats;      ///< Supported surface formats (color space + pixel format).
    std::vector<vk::PresentModeKHR>     presentModes; ///< Supported presentation modes (e.g. FIFO, Mailbox).
  };

  /**
   * @brief Main Vulkan rendering device implementation.
   *
   * VKDevice is the central backend for the Vulkan renderer. It owns and
   * manages the full Vulkan resource hierarchy:
   *
   * - **Instance & surface** — Vulkan instance, debug messenger, window surface.
   * - **Device** — physical device selection, logical device, graphics/present queues.
   * - **Swapchain** — image views, framebuffers, and recreation on resize.
   * - **Render pass** — default render pass for swapchain rendering.
   * - **Command infrastructure** — command pool and per-frame command buffers.
   * - **Synchronization** — semaphores and fences for CPU-GPU and frame overlap.
   * - **Resource factories** — shaders, pipelines, vertex/index buffers, textures, framebuffers.
   *
   * It also drives the per-frame acquire -> record -> submit -> present loop
   * through beginFrame / endFrame (called by @ref VKContext).
   *
   * @note Copy and move operations are disabled; there must be exactly
   *       one VKDevice per rendering context.
   */
  class VKDevice : public GraphicsDevice {
  public:

    /**
     * @brief Constructs the Vulkan device for the given window.
     *
     * Runs the full initialization sequence: instance -> surface ->
     * physical device -> logical device -> swapchain -> render pass ->
     * framebuffers -> command pool/buffers -> sync objects.
     *
     * @param window  Platform window that provides the Vulkan surface.
     *                Must remain valid for the entire lifetime of this device.
     */
    VKDevice(Window* window);

    /**
     * @brief Destructor. Call cleanup() before destruction to release GPU resources.
     */
    ~VKDevice() override = default;

    VKDevice(const VKDevice&) = delete;
    VKDevice& operator=(const VKDevice&) = delete;
    VKDevice(VKDevice&&) noexcept = delete;
    VKDevice& operator=(VKDevice&&) noexcept = delete;

    /** @brief Creates an empty shader object ready to be loaded. */
    std::unique_ptr<Shader>       createShader() override;

    /** @brief Creates an empty pipeline object ready to be configured. */
    std::unique_ptr<Pipeline>     createPipeline() override;

    /**
     * @brief Creates an empty vertex buffer with a preallocated slot count.
     * @param count  Number of vertex slots to reserve (0 = no preallocation).
     */
    std::unique_ptr<VertexBuffer> createVertexBuffer(unsigned int count = 0) override;

    /**
     * @brief Creates a vertex buffer and uploads initial data.
     * @param vertex  Pointer to the vertex data to upload.
     * @param count   Number of bytes to upload.
     */
    std::unique_ptr<VertexBuffer> createVertexBuffer(void* vertex, unsigned int count) override;

    /**
     * @brief Creates an empty index buffer.
     * @param size  Number of index elements to reserve (0 = no preallocation).
     */
    std::unique_ptr<IndexBuffer>  createIndexBuffer(unsigned int size = 0) override;

    /**
     * @brief Creates an index buffer and uploads initial data.
     * @param index  Pointer to the index data to upload.
     * @param size   Number of bytes to upload.
     */
    std::unique_ptr<IndexBuffer>  createIndexBuffer(void* index, unsigned int size) override;

    /**
     * @brief Creates a Vulkan framebuffer with the given specification.
     * @param spec  Framebuffer specification (dimensions, attachment formats).
     */
    std::unique_ptr<FrameBuffer>  createFrameBuffer(const FrameBufferSpec& spec) override;

    /** @brief Creates a vertex array object for binding vertex/index buffers. */
    std::unique_ptr<VertexArray>  createVertexArray() override;

    /**
     * @brief Creates a geometry buffer (G-buffer) for deferred rendering.
     * @param screen_size  Dimensions of the G-buffer in pixels.
     */
    std::unique_ptr<GBuffer>      createGBuffer(glm::vec2 screen_size) override;

    /**
     * @brief Creates a shader storage buffer (SSBO) linked to a pipeline.
     * @param pipeline_id  ID of the pipeline that will own the SSBO.
     */
    std::unique_ptr<ShaderStorageBuffer> createShaderStorageBuffer(u32 pipeline_id) override;

    /**
     * @brief Loads a 2-D texture from disk.
     * @param path  File path to the image (PNG, JPG, etc.).
     */
    std::unique_ptr<Texture> createTexture(const std::string& path) override;

    /**
     * @brief Loads a cubemap texture from six face images.
     * @param path  Array of six file paths in +X/-X/+Y/-Y/+Z/-Z order.
     */
    std::unique_ptr<Texture> createCubemap(const std::array<std::string, 6>& path) override;

    /**
     * @brief Sets the color used to clear the color attachment.
     * @param r  Red component   [0, 1].
     * @param g  Green component [0, 1].
     * @param b  Blue component  [0, 1].
     * @param a  Alpha component [0, 1].
     */
    void setClearColor(float r, float g, float b, float a) override;

    /**
     * @brief Clears the active color and depth buffers.
     *
     * Uses the color set by the most recent setClearColor() call.
     */
    void clearBuffers() override;

    /**
     * @brief Releases all Vulkan resources owned by this device.
     *
     * Must be called before the device is destroyed. Waits for the GPU
     * to become idle, then destroys swapchain, pipelines, sync objects,
     * and the logical device.
     */
    void cleanup() override;

    /**
     * @brief Draws the given mesh using its vertex and index buffers.
     * @param mesh  Mesh to render (must have valid VAO and index count).
     */
    void draw(Mesh& mesh) override;

    /**
     * @brief Issues a non-indexed draw call.
     * @param indices  Number of vertices to draw.
     */
    void draw(uint32_t indices) override;

    /**
     * @brief Issues an indexed draw call.
     * @param indices  Number of indices to draw.
     */
    void drawIndexed(uint32_t indices) override;

    /**
     * @brief Issues an instanced draw call for the given mesh.
     * @param mesh           Mesh to render.
     * @param instanceCount  Number of instances to draw.
     */
    void drawInstanced(Mesh& mesh, u32 instanceCount) override;

    void enableDepthTest()    override; ///< Enables depth testing for subsequent draw calls.
    void disableDepthTest()   override; ///< Disables depth testing for subsequent draw calls.
    void enableFaceCulling()  override; ///< Enables back-face culling.
    void disableFaceCulling() override; ///< Disables face culling.

    /**
     * @brief Returns the command buffer for the current frame.
     *
     * Valid only between beginFrame() and endFrame() calls from @ref VKContext.
     *
     * @return The active VkCommandBuffer.
     */
    vk::CommandBuffer getCommandBuffer();

    /**
     * @brief Sets the render pass that newly created or used pipelines must be compatible with.
     * @param rp  Active VkRenderPass.
     */
    void setCurrentRenderPass(vk::RenderPass rp) { currentRenderPass = rp; }

    /**
     * @brief Returns the currently active render pass.
     * @return Handle to the current VkRenderPass.
     */
    vk::RenderPass getCurrentRenderPass() const { return currentRenderPass; }

    /**
     * @brief Sets the number of color attachments for the active render pass.
     *
     * Clamped to a minimum of 1. Used by pipelines when setting up their
     * color-blend state arrays.
     *
     * @param count  Number of color attachments (>= 1).
     */
    void setCurrentColorAttachmentCount(u32 count) {
      currentColorAttachmentCount_ = std::max<u32>(1, count);
    }

    /**
     * @brief Returns the current color attachment count.
     * @return Number of color attachments in the active render pass.
     */
    u32 getCurrentColorAttachmentCount() const {
      return currentColorAttachmentCount_;
    }

    /**
     * @brief Registers a pipeline so it can be recreated on swapchain resize.
     * @param pipe  Shared pointer to the pipeline to register.
     */
    void registerPipeline(std::shared_ptr<VKPipeline> pipe);

    /**
     * @brief Removes a pipeline from the registry.
     * @param pipe  Raw pointer used to identify the pipeline to remove.
     */
    void unregisterPipeline(VKPipeline* pipe);

    /**
     * @brief Sets the pipeline that will be used for the next draw call.
     * @param pipe  Shared pointer to the pipeline to activate.
     */
    void setCurrentPipeline(std::shared_ptr<Pipeline> pipe);

    Window* window_; ///< Non-owning pointer to the platform window.

    vk::UniqueInstance    instance;      ///< Vulkan instance.
    vk::UniqueDevice      device;        ///< Logical device.
    vk::SurfaceKHR        surface;       ///< Window surface.
    vk::SwapchainKHR      swapChain;     ///< Active swapchain.
    vk::PhysicalDevice    physicalDevice; ///< Selected physical GPU.
    vk::Format            swapChainImageFormat; ///< Pixel format of swapchain images.
    vk::Extent2D          swapChainExtent;      ///< Resolution of the swapchain.
    vk::RenderPass        renderPass;           ///< Default swapchain render pass.
    vk::PipelineLayout    pipelineLayout;       ///< Default pipeline layout.
    vk::Pipeline          graphicsPipeline;     ///< Default graphics pipeline.
    vk::CommandPool       commandPool;          ///< Pool used to allocate command buffers.
    vk::Queue             graphicsQueue;        ///< Graphics submission queue.
    vk::Queue             presentQueue;         ///< Presentation queue.
    vk::Buffer            vertexBuffer;         ///< Default vertex buffer (example use).
    vk::DeviceMemory      vertexBufferMemory;   ///< Memory backing the default vertex buffer.
    vk::Buffer            indexBuffer;          ///< Default index buffer (example use).
    vk::DeviceMemory      indexBufferMemory;    ///< Memory backing the default index buffer.

    std::vector<vk::Image>        swapChainImages;       ///< Swapchain image handles.
    std::vector<vk::ImageView>    swapChainImageViews;   ///< Image views for each swapchain image.
    std::vector<vk::Framebuffer>  swapChainFramebuffers; ///< Framebuffers for swapchain images.
    std::vector<vk::CommandBuffer> commandBuffers;       ///< Per-frame command buffers.
    std::vector<vk::Semaphore>    imageAvailableSemaphores; ///< Signaled when a swapchain image is ready.
    std::vector<vk::Semaphore>    renderFinishedSemaphores; ///< Signaled when rendering is complete.
    std::vector<vk::Fence>        inFlightFences;           ///< CPU-GPU synchronization fences.

    VkDebugUtilsMessengerEXT callback;  ///< Debug messenger handle (validation layers).
    bool framebufferResized = false;    ///< Set to true by the resize callback; triggers swapchain recreation.
    u32  currentFrame = 0;             ///< Index into per-frame synchronization objects.
    uint32_t imageIndex = 0;           ///< Index of the swapchain image acquired for the current frame.

    vk::RenderPass currentRenderPass = nullptr; ///< Render pass currently in use (default or offscreen).
    u32 currentColorAttachmentCount_ = 1;       ///< Color attachment count for the active render pass.

    std::shared_ptr<VKPipeline> pipeline; ///< Active / default graphics pipeline.

    std::vector<std::weak_ptr<VKPipeline>> registeredPipelines_; ///< All live pipelines (weak refs for resize).

    void createInstance();       ///< Creates the Vulkan instance with required extensions.
    void setupDebugCallback();   ///< Installs the validation-layer debug messenger.
    void createSurface();        ///< Creates the VkSurfaceKHR from the GLFW window.
    void pickPhysicalDevice();   ///< Selects the most suitable VkPhysicalDevice.
    void createLogicalDevice();  ///< Creates the VkDevice and retrieves queue handles.
    void createSwapChain();      ///< Creates the VkSwapchainKHR and its images.
    void createImageViews();     ///< Creates VkImageViews for each swapchain image.
    void createRenderPass();     ///< Creates the default VkRenderPass.
    void createFramebuffers();   ///< Creates VkFramebuffers for each swapchain image.
    void createCommandPool();    ///< Creates the VkCommandPool.
    void createCommandBuffers(); ///< Allocates per-frame VkCommandBuffers.
    void createSyncObjects();    ///< Creates semaphores and fences for frame synchronization.
    void cleanupSwapChain();     ///< Destroys swapchain-dependent resources.
    void recreateSwapChain();    ///< Recreates the swapchain and all dependent resources on resize.

    /**
     * @brief Allocates a Vulkan buffer and its backing device memory.
     *
     * @param size          Size of the buffer in bytes.
     * @param usage         Intended usage flags (e.g. vertex, index, transfer).
     * @param properties    Required memory property flags (e.g. host-visible, device-local).
     * @param buffer        [out] Created buffer handle.
     * @param bufferMemory  [out] Allocated device memory handle.
     */
    void createBuffer(vk::DeviceSize size,
      vk::BufferUsageFlags usage,
      vk::MemoryPropertyFlags properties,
      vk::Buffer& buffer,
      vk::DeviceMemory& bufferMemory);

    /**
     * @brief Copies @p size bytes from @p srcBuffer to @p dstBuffer using a transfer command.
     *
     * Submits a one-time command buffer to the graphics queue and waits for completion.
     *
     * @param srcBuffer  Source buffer handle.
     * @param dstBuffer  Destination buffer handle.
     * @param size       Number of bytes to copy.
     */
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    /**
     * @brief Opens a one-time-submit command buffer for short GPU operations.
     *
     * Must be paired with endSingleTimeCommands().
     *
     * @return Allocated and begun VkCommandBuffer.
     */
    vk::CommandBuffer beginSingleTimeCommands();

    /**
     * @brief Ends and submits a one-time command buffer, then waits for completion.
     *
     * @param commandBuffer  Command buffer previously obtained from beginSingleTimeCommands().
     */
    void endSingleTimeCommands(vk::CommandBuffer commandBuffer);

    /** @brief Returns true if validation layers are available on this system. */
    bool checkValidationLayerSupport();

    /** @brief Returns the list of instance extensions required for this platform. */
    std::vector<const char*> getRequiredExtensions();

    /**
     * @brief Creates the debug utils messenger extension object.
     *
     * This must be called manually because the function is an extension
     * and is not part of the core dispatch table.
     */
    VkResult CreateDebugUtilsMessengerEXT(
      VkInstance instance,
      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
      const VkAllocationCallbacks* pAllocator,
      VkDebugUtilsMessengerEXT* pCallback);

    /**
     * @brief Destroys the debug utils messenger extension object.
     */
    void DestroyDebugUtilsMessengerEXT(
      VkInstance instance,
      VkDebugUtilsMessengerEXT callback,
      const VkAllocationCallbacks* pAllocator);

    /**
     * @brief Returns true if the given physical device meets all engine requirements.
     *
     * Checks for required queue families, device extensions, and swapchain support.
     *
     * @param device  Physical device to evaluate.
     */
    bool isDeviceSuitable(const vk::PhysicalDevice& device);

    /**
     * @brief Queries and returns queue family indices for the given physical device.
     * @param device  Physical device to query.
     * @return        Populated QueueFamilyIndices (may be incomplete if unsupported).
     */
    QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device);

    /**
     * @brief Checks whether the device supports all required extensions.
     * @param device  Physical device to check.
     * @return True if all required extensions are supported.
     */
    bool checkDeviceExtensionSupport(const vk::PhysicalDevice& device);

    /**
     * @brief Queries swapchain capabilities for the given physical device.
     * @param device  Physical device to query.
     * @return        Populated SwapChainSupportDetails.
     */
    SwapChainSupportDetails querySwapChainSupport(const vk::PhysicalDevice& device);

    /**
     * @brief Selects the best available surface format.
     *
     * Prefers VK_FORMAT_B8G8R8A8_SRGB with VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
     * falls back to the first available format.
     *
     * @param availableFormats  List of formats reported by the driver.
     */
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);

    /**
     * @brief Selects the best available presentation mode.
     *
     * Prefers Mailbox (triple buffering) over FIFO (vsync).
     *
     * @param availablePresentModes  List of modes reported by the driver.
     */
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> availablePresentModes);

    /**
     * @brief Determines the swap extent (resolution) for the swapchain.
     *
     * Clamps the framebuffer size reported by GLFW to the surface capabilities.
     *
     * @param capabilities  Surface capabilities from the physical device.
     */
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);

    /**
     * @brief Creates a VkShaderModule from pre-compiled SPIR-V bytecode.
     * @param code  Raw SPIR-V bytes.
     * @return      Created shader module.
     */
    vk::ShaderModule createShaderModule(const std::vector<char>& code);

    /**
     * @brief Finds a device memory type that satisfies the given filter and properties.
     *
     * @param typeFilter  Bitmask of memory type indices reported by vkGetBufferMemoryRequirements.
     * @param properties  Required memory property flags.
     * @return            Index of a suitable memory type.
     * @throws std::runtime_error if no suitable type is found.
     */
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
  };

} // namespace mam