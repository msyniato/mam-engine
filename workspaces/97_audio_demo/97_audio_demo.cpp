
#include <mam/engine.hpp>

uint16_t w = 800;
uint16_t h = 600;


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

  auto world = worldRegistry->getCurrentWorld();
  auto context = std::make_unique<mam::Context>();

  auto sisland_mesh = std::make_unique<mam::Mesh>(*graphicsDevice, "SIsland");
  sisland_mesh->LoadOBJ("../assets/models/SIsland.obj");

  auto misland_mesh = std::make_unique<mam::Mesh>(*graphicsDevice, "MIsland");
  misland_mesh->LoadOBJ("../assets/models/MIsland.obj");

  auto lisland_mesh = std::make_unique<mam::Mesh>(*graphicsDevice, "LIsland");
  lisland_mesh->LoadOBJ("../assets/models/LIsland.obj");

  auto* pbr_mat = materialRegistry->getMaterial("pbr_mat");

  mam::Entity SIsland = world->createEntity("SIsland");
  world->addComponent<mam::RenderComponent>(SIsland,
    mam::RenderComponent{
      .mesh = sisland_mesh.get(),
      .material = pbr_mat
    }
  );

  mam::Entity MIsland = world->createEntity("MIsland");
  world->addComponent<mam::RenderComponent>(MIsland,
    mam::RenderComponent{
      .mesh = misland_mesh.get(),
      .material = pbr_mat
    }
  );

  mam::Entity MIsland2 = world->createEntity("MIsland2");
  world->addComponent<mam::RenderComponent>(MIsland2,
    mam::RenderComponent{
      .mesh = misland_mesh.get(),
      .material = pbr_mat
    }
  );

  mam::Entity LIsland = world->createEntity("LIsland");
  world->addComponent<mam::RenderComponent>(LIsland,
    mam::RenderComponent{
      .mesh = lisland_mesh.get(),
      .material = pbr_mat
    }
  );

  auto sIslandTr = world->getComponent<mam::TransformComponent>(SIsland);
  sIslandTr->position = glm::vec3(0.0f, 0.0f, -40.0f);
  sIslandTr->rotation = glm::vec3(0.0f);
  sIslandTr->scale = glm::vec3(5.0f);

  auto mIslandTr = world->getComponent<mam::TransformComponent>(MIsland);
  mIslandTr->position = glm::vec3(40.0f, 0.0f, 0.0f);
  mIslandTr->rotation = glm::vec3(0.0f);
  mIslandTr->scale = glm::vec3(5.0f);

  auto mIsland2Tr = world->getComponent<mam::TransformComponent>(MIsland2);
  mIsland2Tr->position = glm::vec3(0.0f, 0.0f, 40.0f);
  mIsland2Tr->rotation = glm::vec3(0.0f);
  mIsland2Tr->scale = glm::vec3(5.0f);

  auto lIslandTr = world->getComponent<mam::TransformComponent>(LIsland);
  lIslandTr->position = glm::vec3(-40.0f, 0.0f, 0.0f);
  lIslandTr->rotation = glm::vec3(0.0f);
  lIslandTr->scale = glm::vec3(5.0f);

  glm::vec3 target = sIslandTr->position;

  glm::vec2 windowSize = {
    static_cast<float>(window->getWidth()),
    static_cast<float>(window->getHeight())
  };

  camera->initViewTarget(target, windowSize);

  mam::AudioManager audio;
  audio.CreateContext();
  audio.SetListener();

  mam::SourceData musicSrc{};
  musicSrc.pitch = 1.0f;
  musicSrc.gain = 1.0f;
  musicSrc.position = mIslandTr->position;
  musicSrc.velocity = glm::vec3(0.0f);
  musicSrc.loop = true;
  musicSrc.layer = mam::AudioLayer::SFX;
  audio.CreateSource(musicSrc);

  mam::SourceData ambientSrc{};
  ambientSrc.pitch = 1.0f;
  ambientSrc.gain = 1.0f;
  ambientSrc.position = lIslandTr->position;
  ambientSrc.velocity = glm::vec3(0.0f);
  ambientSrc.loop = true;
  ambientSrc.layer = mam::AudioLayer::MUSIC;
  audio.CreateSource(ambientSrc);

  mam::SourceData sfxSrc{};
  sfxSrc.pitch = 1.0f;
  sfxSrc.gain = 1.0f;
  sfxSrc.position = mIsland2Tr->position;
  sfxSrc.velocity = glm::vec3(0.0f);
  sfxSrc.loop = true;
  sfxSrc.layer = mam::AudioLayer::SFX;
  audio.CreateSource(sfxSrc);

  mam::SourceData smallIslandSrc{};
  smallIslandSrc.pitch = 1.0f;
  smallIslandSrc.gain = 1.0f;
  smallIslandSrc.position = sIslandTr->position;
  smallIslandSrc.velocity = glm::vec3(0.0f);
  smallIslandSrc.loop = true;
  smallIslandSrc.layer = mam::AudioLayer::SFX;
  audio.CreateSource(smallIslandSrc);

  const bool oggLoaded = audio.LoadOGGFile("../assets/audio/slbgm_forest_A-01.ogg", 0);
  const bool wavLoaded = audio.LoadWavFile("../assets/audio/acustic_guitar.wav", 1);
  const bool mp3Loaded = audio.LoadMP3File("../assets/audio/badbunny.mp3", 2);
  const bool flacLoaded = audio.LoadFLACFile("../assets/audio/cardigan.flac", 3);

  if (!oggLoaded || !wavLoaded || !mp3Loaded || !flacLoaded) {
    std::cerr << "Could not load one or more audio files." << std::endl;
  }

  audio.PlaySource(0);
  audio.PlaySource(1);
  audio.PlaySource(2);
  audio.PlaySource(3);

  double dt = 0.0;

  while (!window->isClosed()) {
    const double startTime = glfwGetTime();

    camera->update(dt, windowSize, true);
    audio.SetListener(camera->position(), camera->front(), camera->up());

    const glm::vec3 listenerPos = camera->position();

    auto computeGain = [](const glm::vec3& listener, const glm::vec3& soundPos) {
      const float dist = glm::distance(listener, soundPos);

      constexpr float kInnerDistance = 6.0f;
      constexpr float kOuterDistance = 20.0f;

      if (dist <= kInnerDistance) return 1.0f;
      if (dist >= kOuterDistance) return 0.0f;

      const float t = (dist - kInnerDistance) / (kOuterDistance - kInnerDistance);
      return 1.0f - t;
      };

    audio.SetSourceGain(0, computeGain(listenerPos, mIslandTr->position));
    audio.SetSourceGain(1, computeGain(listenerPos, lIslandTr->position));
    audio.SetSourceGain(2, computeGain(listenerPos, mIsland2Tr->position));
    audio.SetSourceGain(3, computeGain(listenerPos, sIslandTr->position));

    engine.update(*context);

    const double endTime = glfwGetTime();
    dt = endTime - startTime;
  }

  engine.cleanup();

  return 0;
}