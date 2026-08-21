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
   * @brief Vulkan implementation of a cubemap texture (skybox).
   *
   * Represents an immutable six-faced cubemap used primarily as a skybox.
   * Because the cubemap is treated as a static asset, all mutable Texture
   * operations (filtering, wrapping, mipmap generation, data upload, etc.)
   * are intentionally no-ops; only @c bind() and @c unBind() are meaningful.
   *
   * Construction is disabled via the deleted constructor — instances must
   * be created through the concrete Vulkan subclass that provides the
   * actual image-loading and Vulkan resource allocation.
   *
   * @note Mutating operations inherited from @ref Texture are left as
   *       empty overrides to satisfy the interface contract while making
   *       it clear they do not apply to a static skybox cubemap.
   */
  class VKCubemap : public Texture {
  public:

    /**
     * @brief Construction from a set of six face paths is disabled at this level.
     *
     * The concrete derived class is responsible for loading the images
     * and creating the Vulkan VkImage, VkImageView and VkSampler.
     *
     * @param faces  Array of six file paths in the order: +X, -X, +Y, -Y, +Z, -Z.
     */
    explicit VKCubemap(const std::array<std::string, 6>& faces) = delete;

    /**
     * @brief Destructor. Derived class is responsible for releasing Vulkan resources.
     */
    ~VKCubemap() override = default;

    /**
     * @brief Binds the cubemap to the given texture slot.
     *
     * Records the descriptor update or push-constant required to make
     * this cubemap available to the currently bound pipeline.
     *
     * @param slot  Texture unit / descriptor binding index (default 0).
     */
    void bind(u32 slot = 0) const override = 0;

    /**
     * @brief Unbinds the cubemap from the given texture slot.
     *
     * @param slot  Texture unit / descriptor binding index (default 0).
     */
    void unBind(u32 slot = 0) const override = 0;

    // -----------------------------------------------------------------------
    // No-op overrides — not applicable to a static skybox cubemap
    // -----------------------------------------------------------------------

    /** @brief No-op: cubemap data is immutable after construction. */
    void setData(const void*, u32, u32, u32) override {}

    /** @brief No-op: cubemap data is immutable after construction. */
    void setData(const void*, u32) override {}

    /** @brief No-op: cubemap source cannot be hot-swapped. */
    void resetTexture(const std::string&, bool = true) override {}

    /** @brief No-op: sampler parameters are fixed at creation time. */
    void setMinFilter(Filter) override {}

    /** @brief No-op: sampler parameters are fixed at creation time. */
    void setMagFilter(Filter) override {}

    /** @brief No-op: wrap mode is fixed at creation time. */
    void setWrapS(Wrap) override {}

    /** @brief No-op: wrap mode is fixed at creation time. */
    void setWrapT(Wrap) override {}

    /** @brief No-op: wrap mode is fixed at creation time. */
    void setWrapR(Wrap) override {}

    /** @brief No-op: wrap mode is fixed at creation time. */
    void setWrap(Wrap) override {}

    /** @brief No-op: mipmaps are generated (or not) at load time. */
    void generateMipmaps() override {}
  };

} // namespace mam