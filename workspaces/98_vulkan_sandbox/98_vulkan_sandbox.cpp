#include "mam/engine.hpp"

#include <array>
#include <iostream>
#include <memory>

namespace {

  constexpr float kHunterSpacing = 25.0f;

  void setupHunterTransform(mam::World* world,
    mam::Entity entity, const glm::vec3& position, 
    const glm::vec3& rotation, const glm::vec3& scale)
  {
    auto* transform = world->getComponent<mam::TransformComponent>(entity);
    if (!transform) return;

    transform->position = position;
    transform->rotation = rotation;
    transform->scale = scale;
  }

  mam::Entity createHunter(mam::World* world,
    const char* name,
    mam::Mesh* mesh,
    mam::Material* material,
    mam::Texture* albedo,
    mam::Texture* normal,
    mam::Texture* roughness,
    mam::Texture* metallic,
    const glm::vec3& position,
    const glm::vec3& rotation,
    const glm::vec3& scale)
  {
    mam::Entity entity = world->createEntity(name);

    world->addComponent<mam::RenderComponent>(
      entity,
      mam::RenderComponent{
        .mesh = mesh,
        .material = material,
        .textures = { albedo, normal, roughness, metallic }
      }
    );

    setupHunterTransform(world, entity, position, rotation, scale);
    return entity;
  }

} // namespace

int main(int argc, char** argv)
{
  mam::GraphicsContext::Desc desc = {
    1280,
    720,
    mam::GraphicsContext::RenderAPI::Vulkan
  };

  mam::Engine engine(desc);
  engine.init();

  auto worldRegistry = engine.getWorldRegistry();
  auto materialRegistry = engine.getMaterialRegistry();
  auto window = engine.getWindow();
  auto graphicsDevice = engine.getGraphicsContext()->getGraphicsDevice();
  auto camera = engine.getCamera();
  auto world = worldRegistry->getCurrentWorld();

  if (!worldRegistry || !materialRegistry || !window || !graphicsDevice || !camera || !world) {
    std::cerr << "98_vulkan_sandbox: engine initialization failed\n";
    engine.cleanup();
    return -1;
  }

  auto hunterMesh = std::make_unique<mam::Mesh>(*graphicsDevice, "hunter_mesh_vk");
  hunterMesh->LoadOBJ("../assets/models/hunter.obj");

  auto hunterAlbedo = graphicsDevice->createTexture("../assets/textures/hunter_albedo.jpeg");
  auto hunterNormal = graphicsDevice->createTexture("../assets/textures/hunter_normal.jpeg");
  auto hunterRoughness = graphicsDevice->createTexture("../assets/textures/hunter_roughness.jpeg");
  auto hunterMetallic = graphicsDevice->createTexture("../assets/textures/hunter_metallic.png");

  mam::Material* pbrMaterial = materialRegistry->getMaterial("pbr_mat");

  if (!pbrMaterial) {
    std::cerr << "98_vulkan_sandbox: pbr_mat not found. "
      << "Check Vulkan PBR module registration in framework.cpp\n";
    engine.cleanup();
    return -1;
  }

  pbrMaterial->setParameter("u_roughness", 0.35f);
  pbrMaterial->setParameter("u_metallic", 0.0f);
  pbrMaterial->setParameter("u_baseColor", glm::vec3(1.0f));

  createHunter(
    world,
    "Hunter_AlbedoOnly",
    hunterMesh.get(),
    pbrMaterial,
    hunterAlbedo.get(),
    nullptr,
    nullptr,
    nullptr,
    glm::vec3(-2.0f * kHunterSpacing, 0.0f, 0.0f),
    glm::vec3(-90.0f, -25.0f, 90.0f),
    glm::vec3(0.85f)
  );

  createHunter(
    world,
    "Hunter_AlbedoNormal",
    hunterMesh.get(),
    pbrMaterial,
    hunterAlbedo.get(),
    hunterNormal.get(),
    nullptr,
    nullptr,
    glm::vec3(-1.0f * kHunterSpacing, 0.0f, 4.0f),
    glm::vec3(-90.0f, -12.0f, 90.0f),
    glm::vec3(1.05f)
  );

  createHunter(
    world,
    "Hunter_AlbedoNormalRoughness",
    hunterMesh.get(),
    pbrMaterial,
    hunterAlbedo.get(),
    hunterNormal.get(),
    hunterRoughness.get(),
    nullptr,
    glm::vec3(0.0f, 0.0f, -3.0f),
    glm::vec3(-90.0f, 0.0f, 90.0f),
    glm::vec3(0.75f, 1.25f, 0.75f)
  );

  createHunter(
    world,
    "Hunter_AlbedoNormalMetallic",
    hunterMesh.get(),
    pbrMaterial,
    hunterAlbedo.get(),
    hunterNormal.get(),
    nullptr,
    hunterMetallic.get(),
    glm::vec3(1.0f * kHunterSpacing, 0.0f, 4.0f),
    glm::vec3(-90.0f, 15.0f, 90.0f),
    glm::vec3(1.15f)
  );

  createHunter(
    world,
    "Hunter_FullPBR",
    hunterMesh.get(),
    pbrMaterial,
    hunterAlbedo.get(),
    hunterNormal.get(),
    hunterRoughness.get(),
    hunterMetallic.get(),
    glm::vec3(2.0f * kHunterSpacing, 0.0f, 0.0f),
    glm::vec3(-90.0f, 30.0f, 90.0f),
    glm::vec3(0.9f, 1.1f, 0.9f)
  );

  mam::LightComponent keyLight = {
    .constant_att = 1.0f,
    .ambient = glm::vec3(0.03f),
    .linear_att = 0.045f,
    .diffuse = glm::vec3(8.0f, 7.5f, 6.8f),
    .quadratic_att = 0.0075f,
    .specular = glm::vec3(8.0f),
    .cut_off = glm::cos(glm::radians(25.0f)),
    .outer_cut_off = glm::cos(glm::radians(35.0f)),
    .type = mam::LightType::PointLight
  };

  mam::LightComponent fillLight = {
    .constant_att = 1.0f,
    .ambient = glm::vec3(0.01f),
    .linear_att = 0.07f,
    .diffuse = glm::vec3(1.2f, 1.4f, 2.0f),
    .quadratic_att = 0.017f,
    .specular = glm::vec3(1.2f, 1.4f, 2.0f),
    .cut_off = glm::cos(glm::radians(25.0f)),
    .outer_cut_off = glm::cos(glm::radians(35.0f)),
    .type = mam::LightType::PointLight
  };

  auto keyLightEntity = world->createEntity("KeyLight_Vulkan");
  auto* keyLightTransform = world->getComponent<mam::TransformComponent>(keyLightEntity);
  keyLightTransform->position = glm::vec3(-45.0f, 35.0f, 25.0f);
  world->addComponent<mam::LightComponent>(keyLightEntity, keyLight);

  auto fillLightEntity = world->createEntity("FillLight_Vulkan");
  auto* fillLightTransform = world->getComponent<mam::TransformComponent>(fillLightEntity);
  fillLightTransform->position = glm::vec3(45.0f, 22.0f, -15.0f);
  world->addComponent<mam::LightComponent>(fillLightEntity, fillLight);

  glm::vec2 windowSize = {
    static_cast<float>(window->getWidth()),
    static_cast<float>(window->getHeight())
  };

  const glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
  camera->setPosition(glm::vec3(0.0f, 35.0f, 70.0f));
  camera->initViewTarget(target, windowSize);

  auto context = std::make_unique<mam::Context>();
  engine.update(*context);

  engine.getGraphicsContext()->waitIdle();

  hunterMesh.reset();
  hunterAlbedo.reset();
  hunterNormal.reset();
  hunterRoughness.reset();
  hunterMetallic.reset();

  engine.cleanup();
  return 0;
}