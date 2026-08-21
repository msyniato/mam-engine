#pragma once

#include "render/api/frame_buffer.hpp"
#include <vulkan/vulkan.hpp>

namespace mam {

  class VKTexture;
  class VKDevice;

  /**
   * @brief Vulkan implementation of an off-screen framebuffer.
   *
   * Wraps a @c VkRenderPass, a @c VkFramebuffer, and the color/depth
   * @ref VKTexture attachments defined by a @ref FrameBufferSpec.
   *
   * @note Copy and move operations are disabled to prevent aliasing of
   *       Vulkan handles.
   */
  class VKFramebuffer : public FrameBuffer {
  public:

    /**
     * @brief Constructs and immediately allocates all Vulkan resources.
     *
     * Creates the render pass, attachment images/views, and the framebuffer
     * object according to @p spec.
     *
     * @param device  Non-owning pointer to the active VKDevice. Must remain
     *                valid for the lifetime of this framebuffer.
     * @param spec    Framebuffer specification describing dimensions,
     *                color attachment formats, and whether a depth attachment
     *                is required.
     */
    VKFramebuffer(VKDevice* device, const FrameBufferSpec& spec);

    /**
     * @brief Destructor. Destroys all Vulkan resources via destroyResources().
     */
    ~VKFramebuffer() override;

    VKFramebuffer(const VKFramebuffer&) = delete;
    VKFramebuffer& operator=(const VKFramebuffer&) = delete;
    VKFramebuffer(VKFramebuffer&&) noexcept = delete;
    VKFramebuffer& operator=(VKFramebuffer&&) noexcept = delete;

    /**
     * @brief Marks this framebuffer as the active render target.
     *
     * Updates the device's current render-pass and color-attachment-count
     * state so subsequently created/used pipelines are compatible.
     */
    void bind() const override;

    /**
     * @brief Deactivates this framebuffer as the render target.
     *
     * Restores the device's render-pass state to the default swapchain
     * render pass.
     */
    void unbind() const override;

    /**
     * @brief Resizes the framebuffer to new dimensions.
     *
     * Destroys existing Vulkan resources and recreates them via invalidate().
     *
     * @param width   New width in pixels.
     * @param height  New height in pixels.
     */
    void resize(u32 width, u32 height) override;

    /**
     * @brief Records vkCmdBeginRenderPass into the given command buffer.
     *
     * Sets the clear values from the spec and begins the render pass
     * for this framebuffer.
     *
     * @param cmd  Active Vulkan command buffer in recording state.
     */
    void beginRenderPass(vk::CommandBuffer cmd) const;

    /**
     * @brief Records vkCmdEndRenderPass into the given command buffer.
     *
     * @param cmd  Active Vulkan command buffer in recording state.
     */
    void endRenderPass(vk::CommandBuffer cmd) const;

    /**
     * @brief Returns the Vulkan render pass object.
     * @return Handle to the VkRenderPass.
     */
    vk::RenderPass  renderPass()  const { return renderPass_; }

    /**
     * @brief Returns the Vulkan framebuffer object.
     * @return Handle to the VkFramebuffer.
     */
    vk::Framebuffer framebuffer() const { return framebuffer_; }

    /**
     * @brief Returns the list of Vulkan color attachment textures.
     *
     * Each element corresponds to one color attachment defined in the spec,
     * in the same order. Useful for sampling the framebuffer output in a
     * subsequent render pass.
     *
     * @return Const reference to the vector of VKTexture shared pointers.
     */
    const std::vector<std::shared_ptr<VKTexture>>& getVKColorAttachments() const {
      return vkColorAttachments_;
    }

  protected:

    /**
     * @brief Recreates all Vulkan resources from the current spec.
     *
     * Called after construction and after every resize(). Invokes
     * createRenderPass(), createAttachments(), and createFramebuffer()
     * in that order.
     */
    void invalidate() override;

  private:

    /**
     * @brief Destroys all owned Vulkan handles.
     *
     * Called by the destructor and at the start of invalidate() when
     * resources already exist.
     */
    void destroyResources();

    /**
     * @brief Creates the VkRenderPass from attachment formats in the spec.
     */
    void createRenderPass();

    /**
     * @brief Creates VkImage, VkDeviceMemory, and VkImageView for each attachment.
     */
    void createAttachments();

    /**
     * @brief Creates the VkFramebuffer using the render pass and image views.
     */
    void createFramebuffer();

    /**
     * @brief Converts an abstract attachment Format to its Vulkan equivalent.
     * @param f  Engine-level format enum.
     * @return   Corresponding vk::Format.
     */
    static vk::Format      toVKFormat(Format f);

    /**
     * @brief Returns the expected final image layout for a given attachment format.
     *
     * Color attachments transition to eShaderReadOnlyOptimal so they can be
     * sampled in subsequent passes; depth attachments use eDepthStencilAttachmentOptimal.
     *
     * @param f  Engine-level format enum.
     * @return   Corresponding vk::ImageLayout.
     */
    static vk::ImageLayout finalLayoutFor(Format f);

    vk::RenderPass  renderPass_ = nullptr; ///< Vulkan render pass handle.
    vk::Framebuffer framebuffer_ = nullptr; ///< Vulkan framebuffer handle.

    std::vector<std::shared_ptr<VKTexture>> vkColorAttachments_; ///< Color attachment textures.
    std::shared_ptr<VKTexture>              vkDepthAttachment_;  ///< Optional depth attachment texture.

    VKDevice* vk_device_; ///< Non-owning pointer to the device used for resource allocation.
  };

} // namespace mam