

#include "render/opengl/gl_context.hpp"
#include "render/opengl/gl_device.hpp"

namespace mam
{
  static GLContext* context = nullptr;

  GLContext::GLContext() {
    graphicsDevice_ = std::make_unique<GLDevice>();
    assert(graphicsDevice_ && "Failed to create graphics device!");

    context = this;
  }

  GraphicsDevice* GLContext::getGraphicsDevice() {
    return graphicsDevice_.get();
  }

  void GLContext::init(Window* window) {
    
  }

  void GLContext::beginFrame() {
    
  }
  
  void GLContext::endFrame() {
    
  }

  void GLContext::shutdown() {
    
  }

}