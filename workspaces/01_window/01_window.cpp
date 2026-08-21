

#include <mam/engine.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h> 

int main(int argc, char **argv)
{

  mam::GraphicsContext::Desc desc = {
    800,
    600,
    mam::GraphicsContext::RenderAPI::OpenGL
  };
  
  mam::Engine engine(desc);
  engine.init();

  auto window = engine.getWindow();
  auto graphicsDevice = engine.getGraphicsContext()->getGraphicsDevice();

  glm::vec2 window_size = {
  static_cast<float>(window->getWidth()),
  static_cast<float>(window->getHeight())
  };

  auto context = std::make_unique<mam::Context>();
  engine.update(*context);

  engine.cleanup();

  return 0;
}