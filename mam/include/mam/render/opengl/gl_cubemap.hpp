#pragma once

#include "render/api/texture.hpp"
#include <array>

namespace mam {

  /**
   * @brief OpenGL cubemap texture loaded from six face images.
   *
   * Implements only bind/unBind; all other Texture virtual methods are no-ops
   * because a static skybox cubemap does not support runtime data updates.
   */
  class GLCubemap : public Texture {
  public:
    /**
     * @brief Load and upload a cubemap from six face image files.
     * @param faces Six file paths in (+X, -X, +Y, -Y, +Z, -Z) order.
     */
    explicit GLCubemap(const std::array<std::string, 6>& faces);
    ~GLCubemap() override;

    /**
     * @brief Bind the cubemap to the given texture unit.
     * @param slot Texture unit index (default 0).
     */
    void bind(u32 slot = 0)   const override;

    /**
     * @brief Unbind the cubemap from the given texture unit.
     * @param slot Texture unit index (default 0).
     */
    void unBind(u32 slot = 0) const override;

    void setData(const void*, u32, u32, u32) override {}
    void setData(const void*, u32)           override {}
    void resetTexture(const std::string&, bool = true) override {}
    void setMinFilter(Filter) override {}
    void setMagFilter(Filter) override {}
    void setWrapS(Wrap)       override {}
    void setWrapT(Wrap)       override {}
    void setWrapR(Wrap)       override {}
    void setWrap(Wrap)        override {}
    void generateMipmaps()    override {}
  };

} // namespace mam
