
#pragma once

#include "render/api/texture.hpp"

namespace mam
{
  /**
   * @brief OpenGL implementation of a Texture
   *
   * Supports 2D textures with filtering, wrapping, mipmaps, and data upload.
   */
  class GLTexture : public Texture
  {
  public:
    /**
     * @brief Construct a texture from a file path
     * @param filepath Path to the texture file
     * @param relative Whether the path is relative to the project root
     */
    GLTexture(const std::string& filepath, bool relative);

    /**
     * @brief Construct an empty texture with specified size and properties
     * @param width Width of the texture
     * @param height Height of the texture
     * @param properties Texture settings (filter, wrap, format)
     */
    GLTexture(const u32 width, const u32 height, const TextureSettings& properties);

    /**
     * @brief Destroy the GLTexture object and release GPU resources
     */
    ~GLTexture() override;

    /**
     * @brief Set the texture data with format and type
     * @param data Pointer to the pixel data
     * @param size Width/Height placeholder (ignored here)
     * @param fmt OpenGL format (GL_RGB, GL_RGBA, etc.)
     * @param type OpenGL type (GL_UNSIGNED_BYTE, etc.)
     */
    void setData(const void* data, u32, u32 glFormat, u32 glType) override;

    /**
     * @brief Set the raw texture data in bytes
     * @param data Pointer to raw data
     * @param sizeBytes Size in bytes
     */
    void setData(const void* data, u32 sizeBytes) override;

    /**
     * @brief Reload or reset the texture from a file
     * @param filepath Path to the texture file
     * @param relative Whether the path is relative to project root
     */
    void resetTexture(const std::string& filepath, bool relative = true) override;

    /**
     * @brief Bind the texture to a specific slot
     * @param slot Texture unit to bind to
     */
    void bind(u32 slot = 0) const override;

    /**
     * @brief Unbind the texture from a specific slot
     * @param slot Texture unit to unbind
     */
    void unBind(u32 slot = 0) const override;

    /**
     * @brief Set the minifying filter for the texture
     * @param f Filter type
     */
    void setMinFilter(Filter f) override;

    /**
     * @brief Set the magnifying filter for the texture
     * @param f Filter type
     */
    void setMagFilter(Filter f) override;

    /**
     * @brief Set texture wrap mode for S coordinate
     * @param c Wrap mode
     */
    void setWrapS(Wrap c) override;

    /**
     * @brief Set texture wrap mode for T coordinate
     * @param c Wrap mode
     */
    void setWrapT(Wrap c) override;

    /**
     * @brief Set texture wrap mode for R coordinate
     * @param c Wrap mode
     */
    void setWrapR(Wrap c) override;

    /**
     * @brief Set texture wrap mode for all coordinates
     * @param c Wrap mode
     */
    void setWrap(Wrap c) override;

    /**
     * @brief Generate mipmaps for the texture
     */
    void generateMipmaps() override;

  private:
    static GLenum toGLFormat(Format f);
    static GLenum toGLInternalFormat(Format f);
    static GLenum toGLWrap(Wrap c);
    static GLenum toGLFilter(Filter f);
    static bool   wantsMipmaps(Filter f);
    static int    calcMipLevels(int w, int h);

    void applyStoredSettings();
  };

}