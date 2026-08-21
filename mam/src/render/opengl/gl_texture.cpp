#include "render/opengl/gl_texture.hpp"

namespace mam
{

#pragma region Helpers
  GLenum GLTexture::toGLFormat(Format f) {
    switch (f) {
    case Format::RGBA8:           return GL_RGBA;
    case Format::RGBA16F:         return GL_RGBA;
    case Format::RGBA32F:         return GL_RGBA;
    case Format::RG32F:           return GL_RG;
    case Format::DEPTH32F:        return GL_DEPTH_COMPONENT;
    case Format::DEPTH24STENCIL8: return GL_DEPTH_STENCIL;
    }
    return GL_RGBA;
  }

  GLenum GLTexture::toGLInternalFormat(Format f) {
    switch (f) {
    case Format::RGBA8:           return GL_RGBA8;
    case Format::RGBA16F:         return GL_RGBA16F;
    case Format::RGBA32F:         return GL_RGBA32F;
    case Format::RG32F:           return GL_RG32F;
    case Format::DEPTH32F:        return GL_DEPTH_COMPONENT32F;
    case Format::DEPTH24STENCIL8: return GL_DEPTH24_STENCIL8;
    }
    return GL_RGBA8;
  }

  GLenum GLTexture::toGLWrap(Wrap c) {
    switch (c) {
    case Wrap::REPEAT:          return GL_REPEAT;
    case Wrap::MIRRORED_REPEAT: return GL_MIRRORED_REPEAT;
    case Wrap::CLAMP_TO_EDGE:   return GL_CLAMP_TO_EDGE;
    }
    return GL_REPEAT;
  }

  GLenum GLTexture::toGLFilter(Filter f) {
    switch (f) {
    case Filter::NEAREST:                return GL_NEAREST;
    case Filter::LINEAR:                 return GL_LINEAR;
    case Filter::NEAREST_MIPMAP_NEAREST: return GL_NEAREST_MIPMAP_NEAREST;
    case Filter::LINEAR_MIPMAP_NEAREST:  return GL_LINEAR_MIPMAP_NEAREST;
    case Filter::NEAREST_MIPMAP_LINEAR:  return GL_NEAREST_MIPMAP_LINEAR;
    case Filter::LINEAR_MIPMAP_LINEAR:   return GL_LINEAR_MIPMAP_LINEAR;
    }
    return GL_LINEAR;
  }

  bool GLTexture::wantsMipmaps(Filter f) {
    return f == Filter::LINEAR_MIPMAP_LINEAR ||
      f == Filter::LINEAR_MIPMAP_NEAREST ||
      f == Filter::NEAREST_MIPMAP_LINEAR ||
      f == Filter::NEAREST_MIPMAP_NEAREST;
  }

  int GLTexture::calcMipLevels(int w, int h) {
    int m = (w > h ? w : h);
    int levels = 1;
    while (m > 1) { m >>= 1; ++levels; }
    return levels;
  }
#pragma endregion

  GLTexture::~GLTexture()
  {
    if (render_id) {
      glDeleteTextures(1, &render_id);
      render_id = 0;
    }
  }

  GLTexture::GLTexture(const std::string& filepath, bool relative)
  {
    setts_.minFilter = Filter::LINEAR_MIPMAP_LINEAR;
    setts_.magFilter = Filter::LINEAR;
    setts_.wrap = Wrap::REPEAT;

    resetTexture(filepath, relative);
  }

  GLTexture::GLTexture(const u32 width, const u32 height, const TextureSettings& properties)
  {
    width_ = width;
    height_ = height;
    path_ = "Texture from memory";
    setts_ = properties;

    const int levels = wantsMipmaps(setts_.minFilter) ? calcMipLevels((int)width_, (int)height_) : 1;

    glCreateTextures(GL_TEXTURE_2D, 1, &render_id);
    glTextureStorage2D(render_id, levels, toGLInternalFormat(setts_.format), width_, height_);

    applyStoredSettings();

    if (levels > 1) {
      glGenerateTextureMipmap(render_id);
      glTextureParameteri(render_id, GL_TEXTURE_MAX_LEVEL, levels - 1);
    }
  }

  void GLTexture::setData(const void* data, u32 sizeBytes,
    u32 glFormat, u32 glType) {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTextureSubImage2D(render_id, 0, 0, 0,
      (GLsizei)width_, (GLsizei)height_,
      (GLenum)glFormat, (GLenum)glType, data);
  }

  void GLTexture::setData(const void* data, u32 sizeBytes) {
    GLenum fmt = toGLFormat(setts_.format);
    GLenum type = GL_UNSIGNED_BYTE;

    switch (setts_.format) {
    case Format::RGBA16F: type = GL_HALF_FLOAT; break;
    case Format::RGBA32F: type = GL_FLOAT;      break;
    case Format::RG32F:   type = GL_FLOAT;      break;
    default:              type = GL_UNSIGNED_BYTE; break;
    }
    setData(data, sizeBytes, (u32)fmt, (u32)type);
  }

  void GLTexture::resetTexture(const std::string& filepath, bool relative)
  {
    if (render_id)
    {
      glDeleteTextures(1, &render_id);
      render_id = 0;
    }

    const std::string resolvedPath = relative ? (filepath) : filepath;
    path_ = filepath;

    stbi_set_flip_vertically_on_load(1);
    int w = 0, h = 0, channels = 0;
    unsigned char* pixels = stbi_load(resolvedPath.c_str(), &w, &h, &channels, 0);
    if (!pixels)
    {
      std::cout << "Texture failed to load: " << filepath << std::endl;
      std::cout << "stb error: " << stbi_failure_reason() << std::endl;
    }
    else
    {
      std::cout << "Texture loaded: " << filepath << " " << w << "x" << h << std::endl;
    }
    bool fallback = (pixels == nullptr || w <= 0 || h <= 0 || channels <= 0);

    if (fallback) {
      w = h = 1;
      channels = 4;
    }

    width_ = static_cast<u32>(w);
    height_ = static_cast<u32>(h);

    GLenum dataFormat = GL_RGBA;
    GLenum internalFormat = GL_RGBA8;

    switch (channels) {
    case 4:
      dataFormat = GL_RGBA;
      internalFormat = GL_RGBA8;
      setts_.format = Format::RGBA8;
      break;
    case 3:
      dataFormat = GL_RGB;
      internalFormat = GL_RGB8;
      setts_.format = Format::RGBA8;
      break;
    case 2:
      dataFormat = GL_RG;
      internalFormat = GL_RG8;
      break;
    case 1:
      dataFormat = GL_RED;
      internalFormat = GL_R8;
      break;
    default:
      dataFormat = GL_RGBA;
      internalFormat = GL_RGBA8;
      break;
    }

    const bool wantsMip = wantsMipmaps(setts_.minFilter);
    const int levels = wantsMip ? calcMipLevels(w, h) : 1;

    glCreateTextures(GL_TEXTURE_2D, 1, &render_id);
    glTextureStorage2D(render_id, levels, internalFormat, width_, height_);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (!fallback) {
      glTextureSubImage2D(render_id, 0, 0, 0, width_, height_, dataFormat, GL_UNSIGNED_BYTE, pixels);
      stbi_image_free(pixels);
    }
    else {
      path_ = "Error Texture";
      const unsigned char magenta[4] = { 255, 0, 255, 255 };
      glTextureSubImage2D(render_id, 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, magenta);
    }

    applyStoredSettings();

    if (wantsMip) {
#ifdef GL_TEXTURE_MAX_ANISOTROPY_EXT
      GLfloat maxAniso = 0.0f;
      glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
      if (maxAniso > 0.0f) {
        const float clamped = (8.0f < maxAniso ? 8.0f : maxAniso);
        glTextureParameterf(render_id, GL_TEXTURE_MAX_ANISOTROPY_EXT, clamped);
      }
#endif
      glGenerateTextureMipmap(render_id);
      glTextureParameteri(render_id, GL_TEXTURE_MAX_LEVEL, levels - 1);
    }
  }

#pragma region Setters
  void GLTexture::setMinFilter(Filter f) {
    setts_.minFilter = f;
    glTextureParameteri(render_id, GL_TEXTURE_MIN_FILTER, toGLFilter(f));
  }

  void GLTexture::setMagFilter(Filter f) {
    setts_.magFilter = f;
    glTextureParameteri(render_id, GL_TEXTURE_MAG_FILTER, toGLFilter(f));
  }

  void GLTexture::setWrapS(Wrap c) {
    setts_.wrap = c;
    glTextureParameteri(render_id, GL_TEXTURE_WRAP_S, toGLWrap(c));
  }

  void GLTexture::setWrapT(Wrap c) {
    setts_.wrap = c;
    glTextureParameteri(render_id, GL_TEXTURE_WRAP_T, toGLWrap(c));
  }

  void GLTexture::setWrapR(Wrap c) {
    setts_.wrap = c;
    glTextureParameteri(render_id, GL_TEXTURE_WRAP_R, toGLWrap(c));
  }

  void GLTexture::setWrap(Wrap c) {
    setWrapS(c);
    setWrapT(c);
    setWrapR(c);
  }
#pragma endregion

  void GLTexture::generateMipmaps() {
    glGenerateTextureMipmap(render_id);
  }


  void GLTexture::bind(u32 slot) const
  {
    glBindTextureUnit(slot, render_id);
  }

  void GLTexture::unBind(u32 slot) const
  {
    glBindTextureUnit(slot, 0);
  }

  void GLTexture::applyStoredSettings() {
    glTextureParameteri(render_id, GL_TEXTURE_MIN_FILTER, toGLFilter(setts_.minFilter));
    glTextureParameteri(render_id, GL_TEXTURE_MAG_FILTER, toGLFilter(setts_.magFilter));
    glTextureParameteri(render_id, GL_TEXTURE_WRAP_S, toGLWrap(setts_.wrap));
    glTextureParameteri(render_id, GL_TEXTURE_WRAP_T, toGLWrap(setts_.wrap));
    glTextureParameteri(render_id, GL_TEXTURE_WRAP_R, toGLWrap(setts_.wrap));
  }

}