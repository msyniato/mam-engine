

#include <mam/engine.hpp>

int main(int argc, char **argv)
{
  mam::GraphicContext::Desc desc;

  mam::MAMEngine engine;
  engine.init(desc);

  mam::Window* window = mam::GraphicContext::getWindow().get();

  while (!window->isClosed())
  {
    // Swap front and back buffers
    engine.update();
  }

  engine.cleanup();

  return 0;
}