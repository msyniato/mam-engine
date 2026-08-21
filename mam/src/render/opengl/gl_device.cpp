
#include "render/opengl/gl_device.hpp"

#include "render/api/mesh.hpp"
#include "render/api/buffer.hpp"
#include "render/opengl/gl_buffer.hpp"

#include "render/api/vertex_array.hpp"
#include "render/opengl/gl_vertex_array.hpp"

#include "render/api/shader.hpp"
#include "render/opengl/gl_shader.hpp"

#include "render/api/pipeline.hpp"
#include "render/opengl/gl_pipeline.hpp"
#include "render/opengl/gl_cubemap.hpp"

#include "render/api/frame_buffer.hpp"
#include "render/opengl/gl_frame_buffer.hpp"
#include "render/opengl/gl_texture.hpp"

namespace mam {
  
  void GLDevice::draw(Mesh &mesh)
  {
    mesh.vertexArray()->bind();
    glDrawElements(GL_TRIANGLES, mesh.indices(), GL_UNSIGNED_INT, 0);
  }

  void GLDevice::draw(u32 indices)
  {
    glDrawArrays(GL_TRIANGLES, 0, 6);
  }

  void GLDevice::drawIndexed(u32 indices) {
    glDrawElements(GL_TRIANGLES, indices, GL_UNSIGNED_INT, 0);
  }

  void GLDevice::cleanup()
  {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
  }

  void GLDevice::enableDepthTest()
  {
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
  }

  void GLDevice::disableDepthTest()   
  {
    glDisable(GL_DEPTH_TEST);
  }

  void GLDevice::enableFaceCulling()
  {
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
  }

  void GLDevice::disableFaceCulling()
  {
	  glDisable(GL_CULL_FACE);
  }
  
  std::unique_ptr<Shader> GLDevice::createShader()
  {
    return std::make_unique<GLShader>();
  }

  std::unique_ptr<VertexBuffer> GLDevice::createVertexBuffer(unsigned int count)
  {
    return std::make_unique<GLVertexBuffer>(count);
  }

  std::unique_ptr<VertexBuffer> GLDevice::createVertexBuffer(void *vertex, unsigned int count)
  {
    return std::make_unique<GLVertexBuffer>(vertex, count);
  }

  std::unique_ptr<IndexBuffer> GLDevice::createIndexBuffer(unsigned int size)
  {
    return std::make_unique<GLIndexBuffer>(size);
  }

  std::unique_ptr<IndexBuffer> GLDevice::createIndexBuffer(void *index, unsigned int size)
  {
    return std::make_unique<GLIndexBuffer>(index, size);
  }

  std::unique_ptr<Pipeline> GLDevice::createPipeline()
  {
    return std::make_unique<GLPipeline>();
  }
  
  std::unique_ptr<FrameBuffer> GLDevice::createFrameBuffer(const FrameBufferSpec& spec) {
    return std::make_unique<GLFramebuffer>(spec);
  }

  void GLDevice::setClearColor(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
  }
  
  void GLDevice::clearBuffers() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }
  
  std::unique_ptr<VertexArray> GLDevice::createVertexArray()
  {
    return std::make_unique<GLVertexArray>();
  }

	std::unique_ptr<GBuffer> GLDevice::createGBuffer(glm::vec2 screen_size)
	{
    return std::make_unique<GLGBuffer>(screen_size);
	}

	std::unique_ptr<ShaderStorageBuffer> GLDevice::createShaderStorageBuffer(u32 pipeline_id)
	{
    return std::make_unique<GLShaderStorageBuffer>(pipeline_id);
	}
  
  std::unique_ptr<Texture> GLDevice::createTexture(const std::string& path) {
    return std::make_unique<GLTexture>(path, true);
  }

  std::unique_ptr<Texture> GLDevice::createCubemap(const std::array<std::string, 6>& path) {
    return std::make_unique<GLCubemap>(path);
	}

  void GLDevice::drawInstanced(Mesh& mesh, u32 instanceCount)
  {
    mesh.vertexArray()->bind();

    static int count = 0;
    if (count++ < 5) {

      GLint vao = 0, prog = 0;
      glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
      glGetIntegerv(GL_CURRENT_PROGRAM, &prog);

      for (int i = 4; i < 8; i++) {
        GLint div = 0, buf = 0;
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_DIVISOR, &div);
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &buf);
        GLint stride = 0, offset = 0;
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &stride);
        GLvoid* ptr = nullptr;
        glGetVertexAttribPointerv(i, GL_VERTEX_ATTRIB_ARRAY_POINTER, &ptr);
      }

      GLenum err = glGetError();
      if (err != GL_NO_ERROR) printf("  [GL err pre-draw]: 0x%x\n", err);
    }

    glDrawElementsInstanced(GL_TRIANGLES, mesh.indices(),
      GL_UNSIGNED_INT, 0, instanceCount);

    if (count <= 5) {
      GLenum err = glGetError();
      if (err != GL_NO_ERROR) printf("  [GL err post-draw]: 0x%x\n", err);
    }
  }
}
