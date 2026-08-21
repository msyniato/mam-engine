#include <mam/engine.hpp>

mam::World* g_world = nullptr;
mam::InputManager* g_input = nullptr;

std::vector<mam::Entity> g_controlledEntities;

float g_moveSpeed = 2.0f;

void triangleControlSystemUpdate(mam::Context& ctx)
{
  if (!g_world || !g_input) return;

  const float dt = static_cast<float>(g_world->lastDeltaTime);

  if (dt <= 0.0f) return;

  glm::vec2 movement{ 0.0f, 0.0f };

  if (g_input->isKeyPressed(mam::Up)) {
    movement.y += 1.0f;
  }

  if (g_input->isKeyPressed(mam::Down)) {
    movement.y -= 1.0f;
  }

  if (g_input->isKeyPressed(mam::Left)) {
    movement.x -= 1.0f;
  }

  if (g_input->isKeyPressed(mam::Right)) {
    movement.x += 1.0f;
  }

  if (glm::length(movement) > 0.0f) {
    movement = glm::normalize(movement);
  }

  for (mam::Entity entity : g_controlledEntities) {
    auto* transform = g_world->getComponent<mam::TransformComponent>(entity);

    if (!transform) continue;

    transform->position += glm::vec3(
      movement.x * g_moveSpeed * dt,
      movement.y * g_moveSpeed * dt,
      0.0f
    );
  }
}

int main(int argc, char** argv)
{
  mam::GraphicsContext::Desc desc = {
    800,
    600,
    mam::GraphicsContext::RenderAPI::OpenGL
  };

  mam::Engine engine(desc);
  engine.init();

  auto worldRegistry = engine.getWorldRegistry();
  auto materialRegistry = engine.getMaterialRegistry();

  auto window = engine.getWindow();
  auto graphicsDevice = engine.getGraphicsContext()->getGraphicsDevice();
  auto camera = engine.getCamera();
  auto input = engine.getInputManager();

  auto world = worldRegistry->getCurrentWorld();
  
  g_world = world;
  g_input = input;

  mam::Signature controlSignature;
  controlSignature.set(world->getComponentType<mam::TransformComponent>());

  auto* controlSystem = world->registerSystem("triangle_control_system", controlSignature);
  controlSystem->onUpdate = triangleControlSystemUpdate;

  std::vector<mam::Vertex> vertices;
  // init vertices
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

  std::vector<unsigned int> indices;
  // init indices
  indices.push_back(0);
  indices.push_back(1);
  indices.push_back(2);

  std::unique_ptr<mam::Mesh> triangle_mesh = std::make_unique<mam::Mesh>(*graphicsDevice, "Triangle", vertices, indices);

  mam::Entity triangle_entity = world->createEntity("TriangleEntity");

  auto* triangleTransform = world->getComponent<mam::TransformComponent>(triangle_entity);

  triangleTransform->position = glm::vec3(0.0f, 0.0f, 0.0f);
  triangleTransform->rotation = glm::vec3(0.0f, 0.0f, 0.0f);
  triangleTransform->scale = glm::vec3(1.0f, 1.0f, 1.0f);

  world->addComponent<mam::RenderComponent>(
    triangle_entity,
    mam::RenderComponent{
      .mesh = triangle_mesh.get(),
      .material = materialRegistry->getMaterial("basic_mat")
    }
  );

  g_controlledEntities.push_back(triangle_entity);

  glm::vec3 target = triangleTransform->position;

  glm::vec2 window_size = {
    static_cast<float>(window->getWidth()),
    static_cast<float>(window->getHeight())
  };

  glm::vec3 initPosition = { 0.0f, 0.0f, 3.0f };

  camera->setPosition(initPosition);
  camera->initViewTarget(target, window_size);

  auto context = std::make_unique<mam::Context>();
  engine.update(*context);

  g_controlledEntities.clear();
  triangle_mesh.reset();

  g_world = nullptr;
  g_input = nullptr;

  engine.cleanup();

  return 0;
}