#include <mam/engine.hpp>

mam::GraphicsDevice* g_gd = nullptr;
mam::JobSystem* g_jobSystem = nullptr;

std::vector<std::unique_ptr<mam::Mesh>> g_meshPool;
std::vector<std::string> g_meshFiles;
std::vector<mam::Entity> g_cycledEntities;

mam::Mesh* g_currentMesh = nullptr;
int g_currentMeshIndex = 0;

std::atomic<bool> g_isMeshLoading{ false };

bool g_changeKeyPrev = false;
bool g_requestNextMesh = false;

void meshCycleSystemUpdate(mam::Context& ctx)
{
  auto* world = ctx.Get<mam::World>();
  auto* input = ctx.Get<mam::InputManager>();

  if (!world || !input || !g_jobSystem || !g_gd) return;
  if (g_meshFiles.empty()) return;

  const bool changeKeyNow = input->isKeyPressed(mam::Space);
  const bool shouldChange = (changeKeyNow && !g_changeKeyPrev) || g_requestNextMesh;

  if (shouldChange && !g_isMeshLoading.load()) {
    g_requestNextMesh = false;
    g_isMeshLoading.store(true);

    const int nextIndex =
      (g_currentMeshIndex + 1) % static_cast<int>(g_meshFiles.size());

    const std::string path = g_meshFiles[nextIndex];

    g_jobSystem->submit_callable([path, nextIndex, world]() {

      mam::Dispatcher::RunOnMain([path, nextIndex, world]() {

        auto newMesh = std::make_unique<mam::Mesh>(*g_gd, "OBJ");
        newMesh->LoadOBJ(path.c_str());

        mam::Mesh* meshPtr = newMesh.get();
        g_meshPool.push_back(std::move(newMesh));

        for (mam::Entity e : g_cycledEntities) {
          auto* rc = world->getComponent<mam::RenderComponent>(e);

          if (rc) {
            rc->mesh = meshPtr;
          }
        }

        g_currentMesh = meshPtr;
        g_currentMeshIndex = nextIndex;

        g_isMeshLoading.store(false);
        });
      });
  }

  g_changeKeyPrev = changeKeyNow;
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
  auto jobsys = engine.getJobSystem();

  auto world = worldRegistry->getCurrentWorld();

  g_gd = graphicsDevice;
  g_jobSystem = jobsys;

  graphicsDevice->enableFaceCulling();

  auto* defaultMat = materialRegistry->getMaterial("basic_mat");

  g_meshFiles = {
    "../assets/models/cube.obj",
    "../assets/models/SM_Suzanne.obj",
    "../assets/models/SM_Teapot.obj"
  };

  auto firstMesh = std::make_unique<mam::Mesh>(*graphicsDevice, "OBJ");
  firstMesh->LoadOBJ(g_meshFiles[0].c_str());

  g_currentMesh = firstMesh.get();
  g_currentMeshIndex = 0;

  g_meshPool.push_back(std::move(firstMesh));

  mam::Signature sig;
  sig.set(world->getComponentType<mam::RenderComponent>());

  auto* meshCycleSystem = world->registerSystem("mesh_cycle_system", sig);
  meshCycleSystem->onUpdate = meshCycleSystemUpdate;

  const glm::vec3 positions[5] = {
    {  0.0f, 0.0f,  0.0f },
    {  2.0f, 0.0f,  2.0f },
    { -2.0f, 0.0f,  2.0f },
    {  2.0f, 0.0f, -2.0f },
    { -2.0f, 0.0f, -2.0f }
  };

  for (int i = 0; i < 5; ++i) {
    mam::Entity cube = world->createEntity("Cube_" + std::to_string(i));

    world->addComponent<mam::RenderComponent>(
      cube,
      mam::RenderComponent{
        .mesh = g_currentMesh,
        .material = defaultMat
      }
    );

    auto* transform = world->getComponent<mam::TransformComponent>(cube);
    transform->position = positions[i];
    transform->rotation = glm::vec3(0.0f);
    transform->scale = glm::vec3(0.5f);

    g_cycledEntities.push_back(cube);
  }

  glm::vec3 target =
    world->getComponent<mam::TransformComponent>(g_cycledEntities[0])->position;

  glm::vec2 windowSize = {
    static_cast<float>(window->getWidth()),
    static_cast<float>(window->getHeight())
  };

  glm::vec3 initPosition = { 0.0f, 5.0f, 8.0f };

  camera->setPosition(initPosition);
  camera->initViewTarget(target, windowSize);

  auto context = std::make_unique<mam::Context>();
  engine.update(*context);

  g_cycledEntities.clear();
  g_currentMesh = nullptr;
  g_meshPool.clear();

  g_gd = nullptr;
  g_jobSystem = nullptr;

  engine.cleanup();

  return 0;
}
