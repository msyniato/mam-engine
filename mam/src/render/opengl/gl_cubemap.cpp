#include "render/opengl/gl_cubemap.hpp"
#include <iostream>

namespace mam {

  GLCubemap::GLCubemap(const std::array<std::string, 6>& faces) {
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &render_id);

    stbi_set_flip_vertically_on_load(0);   

    int w = 0, h = 0, c = 0;
    bool storageDone = false;

    for (int i = 0; i < 6; ++i) {
      unsigned char* px = stbi_load(faces[i].c_str(), &w, &h, &c, 4); 
      if (!px) {
        std::cout << "Cubemap face failed: " << faces[i]
          << " (" << stbi_failure_reason() << ")\n";
        continue;
      }
      if (!storageDone) {
        glTextureStorage2D(render_id, 1, GL_RGBA8, w, h);
        width_ = (u32)w; height_ = (u32)h;
        storageDone = true;
      }
      glTextureSubImage3D(render_id, 0, 0, 0, i, w, h, 1,
        GL_RGBA, GL_UNSIGNED_BYTE, px);
      stbi_image_free(px);
    }

    glTextureParameteri(render_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(render_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(render_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(render_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(render_id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  }

  GLCubemap::~GLCubemap() {
    if (render_id) { glDeleteTextures(1, &render_id); render_id = 0; }
  }

  void GLCubemap::bind(u32 slot) const { glBindTextureUnit(slot, render_id); }
  void GLCubemap::unBind(u32 slot) const { glBindTextureUnit(slot, 0); }

}