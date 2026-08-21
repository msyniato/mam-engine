#include <mam/engine.hpp>

std::vector<mam::Entity> g_cubes;

float g_gravityStrength = 1.0f;
float g_resetHeight = 50.0f;
float g_floorY = 0.0f;

bool  g_pauseSimulation = false;
float g_timeScale = 1.0f;

void gravitySystemUpdate(mam::Context& ctx)
{
  auto* world = ctx.Get<mam::World>();

  if (!world) return;
  if (g_pauseSimulation) return;

  const float dt = static_cast<float>(world->lastDeltaTime) * g_timeScale;

  if (dt <= 0.0f) return;

  auto trType = world->getComponentType<mam::TransformComponent>();

  for (auto& arch : world->getArchetypes()) {
    if (!arch->signature().test(trType)) continue;

    auto* trArr = arch->getArray<mam::TransformComponent>(trType);

    for (size_t i = 0; i < arch->getEntitiesNum(); ++i) {
      auto& transform = trArr->get(i);

      transform.position.y -= g_gravityStrength * dt;

      if (transform.position.y < g_floorY) {
        transform.position.y = g_resetHeight;
      }
    }
  }
}

int main(int argc, char** argv)
{
#pragma region ENGINE_SETUP

  mam::GraphicsContext::Desc desc = {
    1280,
    720,
    mam::GraphicsContext::RenderAPI::OpenGL
  };

  mam::Engine engine(desc);
  engine.init();

  auto worldRegistry = engine.getWorldRegistry();
  auto materialRegistry = engine.getMaterialRegistry();

  auto window = engine.getWindow();
  auto graphicsDevice = engine.getGraphicsContext()->getGraphicsDevice();
  auto camera = engine.getCamera();

  auto world = worldRegistry->getCurrentWorld();

  graphicsDevice->enableFaceCulling();

  auto* defaultMat = materialRegistry->getMaterial("basic_mat");

  mam::Signature gravitySignature;
  gravitySignature.set(world->getComponentType<mam::TransformComponent>());

  auto* gravitySystem = world->registerSystem("gravity_system", gravitySignature);
  gravitySystem->onUpdate = gravitySystemUpdate;

  auto cubeMesh = std::make_unique<mam::Mesh>(*graphicsDevice, "cube_mesh");
  cubeMesh->LoadOBJ("../assets/models/cube.obj");

  constexpr int kCubeCount = 1000;

  g_cubes.reserve(kCubeCount);

  std::mt19937 rng(12345);

  std::uniform_int_distribution<int> distXZ(-100, 100);
  std::uniform_int_distribution<int> distY(0, 50);

  for (int i = 0; i < kCubeCount; ++i) {
    mam::Entity cube = world->createEntity("Cube_" + std::to_string(i));

    auto* transform = world->getComponent<mam::TransformComponent>(cube);

    transform->position = glm::vec3(
      static_cast<float>(distXZ(rng)),
      static_cast<float>(distY(rng)),
      static_cast<float>(distXZ(rng))
    );

    transform->rotation = glm::vec3(0.0f);
    transform->scale = glm::vec3(0.5f);

    world->addComponent<mam::RenderComponent>(
      cube,
      mam::RenderComponent{
        .mesh = cubeMesh.get(),
        .material = defaultMat
      }
    );

    g_cubes.push_back(cube);
  }

  glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);

  glm::vec2 windowSize = {
    static_cast<float>(window->getWidth()),
    static_cast<float>(window->getHeight())
  };

  glm::vec3 initPosition = {
    0.0f,
    40.0f,
    120.0f
  };

  camera->setPosition(initPosition);
  camera->initViewTarget(target, windowSize);

  auto context = std::make_unique<mam::Context>();
  engine.update(*context);

  g_cubes.clear();
  cubeMesh.reset();

  engine.cleanup();

  return 0;
}