
#include "mam/engine.hpp"

int main(int argc, char **argv)
{

  // engine setup
  mam::GraphicsContext::Desc desc = {
    800,
    600,
    mam::GraphicsContext::RenderAPI::OpenGL
  };

  mam::Engine engine(desc);
  engine.init();
  
  auto worldRegistry = engine.getWorldRegistry();
  auto materialRegistry = engine.getMaterialRegistry();
  auto materialModuleRegistry = engine.getMaterialModuleRegistry();

  auto window = engine.getWindow();
  auto renderer = engine.getRenderer();
  auto graphicsDevice = engine.getGraphicsContext()->getGraphicsDevice();
  auto camera = engine.getCamera();

  auto world = worldRegistry->getCurrentWorld(); 
  auto scene = world->getCurrentScene();
  
  // init vertices
  std::vector<mam::Vertex> vertices;
  vertices.push_back({
    { -0.5f, -0.5f * float(sqrt(3)) / 3, 0.0f}, // pos
    {  1.0f,  0.0f,                      0.0f}, // norm
    {  0.0f,  0.0f}                             // texCoord
    });

  vertices.push_back({
    {  0.5f, -0.5f * float(sqrt(3)) / 3, 0.0f}, // pos
    {  1.0f,  0.0f,                      0.0f}, // norm
    {  0.0f,  1.0f}                             // texCoord
    });

  vertices.push_back({
    {  0.0f,  0.5f * float(sqrt(3)) * 2 / 3, 0.0f}, // pos
    {  1.0f,  0.0f,                          0.0f}, // norm
    {  1.0f,  1.0f}                                 // texCoord
    });

  // init indices
  std::vector<unsigned int> indices;
  indices.push_back(0);
  indices.push_back(1);
  indices.push_back(2);

  std::unique_ptr<mam::Mesh> triangle_mesh = std::make_unique<mam::Mesh>(*graphicsDevice, "Triangle", vertices, indices);

  mam::Entity triangle_entity = world->createEntity("TriangleEntity");

  world->addComponent<mam::RenderComponent>(triangle_entity, 
    mam::RenderComponent{
        .mesh = triangle_mesh.get(),
        .material = materialRegistry->getMaterial("basic_mat")
    }
  );

  world->addComponent<mam::TransformComponent>(triangle_entity,
    mam::TransformComponent{
        .position = glm::vec3(0.0, 0.0, 0.0),
        .rotation = glm::vec3(0.0, 0.0, 0.0),
        .scale = glm::vec3(1.0, 1.0, 1.0)
    }
  );
  
  glm::vec3 target = world->getComponent<mam::TransformComponent>(triangle_entity)->position;
  
  glm::vec2 window_size = {
    static_cast<float>(window->getWidth()), 
    static_cast<float>(window->getHeight())
  };
  
  glm::vec3 initPosition = {0.0f, 0.0f, 3.0f};
  
  camera->setPosition(initPosition);
  camera->initViewTarget(target, window_size);

  auto context = std::make_unique<mam::Context>();
  engine.update(*context);
  
  engine.cleanup();

  return 0;
}