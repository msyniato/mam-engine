
#pragma once 

#include "common/common.hpp"

namespace mam {

	class Texture
	{
	public:
		Texture() = default;
		virtual ~Texture() = 0;

    /**
     * @brief Settings for texture filtering, wrapping, and format.
     */
    struct TextureSettings
    {
      Filter minFilter = Filter::NEAREST; ///< Minification filter
      Filter magFilter = Filter::NEAREST; ///< Magnification filter
      Wrap wrap = Wrap::REPEAT;           ///< Texture wrap mode
      Format format = Format::RGBA8;      ///< Texture format
    };

    /**
     * @brief Upload raw data to the texture.
     * @param data Pointer to data
     * @param size Size or width (depending on override)
     * @param fmt Format (OpenGL enum)
     * @param type Type (OpenGL enum)
     */
    virtual void setData(const void* data, u32, u32 glFormat, u32 glType) = 0;

    /**
     * @brief Upload raw data with size in bytes.
     * @param data Pointer to data
     * @param sizeBytes Size of data in bytes
     */
    virtual void setData(const void* data, u32 sizeBytes) = 0;

    /**
     * @brief Reset texture from a file.
     * @param filepath Path to the texture file
     * @param relative True if path is relative to project
     */
    virtual void resetTexture(const std::string& filepath, bool relative = true) = 0;

    /// Set the minification filter
    virtual void setMinFilter(Filter f) = 0;

    /// Set the magnification filter
    virtual void setMagFilter(Filter f) = 0;

    /// Set wrapping mode for S coordinate
    virtual void setWrapS(Wrap c) = 0;

    /// Set wrapping mode for T coordinate
    virtual void setWrapT(Wrap c) = 0;

    /// Set wrapping mode for R coordinate
    virtual void setWrapR(Wrap c) = 0;

    /// Set wrapping mode for all coordinates
    virtual void setWrap(Wrap c) = 0;

    /// Generate mipmaps for the texture
    virtual void generateMipmaps() = 0;

    /// Get texture width
    inline u32 width() { return width_; }

    /// Get texture height
    inline u32 height() { return height_; }

    /**
     * @brief Bind the texture to a slot.
     * @param slot Texture slot
     */
    virtual void bind(u32 slot = 0) const = 0;

    /**
     * @brief Unbind the texture from a slot.
     * @param slot Texture slot
     */
    virtual void unBind(u32 slot = 0) const = 0;

    /// Get GPU texture ID
    inline u32 id() const { return render_id; }

    /**
     * @brief Compare textures by GPU ID.
     * @param other Other texture
     * @return True if both textures have same GPU ID
     */
    bool operator==(const Texture& other) const
    {
      return render_id == other.render_id;
    }

    std::string name_;   ///< Texture name
    std::string path_;   ///< Path to texture file
    std::string shader_; ///< Associated shader

  protected:
    TextureSettings setts_; ///< Texture settings
    u32 width_;             ///< Width in pixels
    u32 height_;            ///< Height in pixels
    u32 depth_;             ///< Depth (for 3D textures)
    u32 render_id;          ///< GPU texture ID

  private:

  };

} // namespace mam