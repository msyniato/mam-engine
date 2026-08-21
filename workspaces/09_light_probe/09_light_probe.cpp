#include "mam/engine.hpp"

int main(int argc, char **argv)
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

  std::unique_ptr<mam::Mesh> hunter_mesh = std::make_unique<mam::Mesh>(*graphicsDevice, "hunter_mesh");
  hunter_mesh->LoadOBJ("../assets/models/hunter.obj");
  
  auto hunter_albedo = graphicsDevice->createTexture("../assets/textures/hunter_albedo.jpeg");
  auto hunter_normal = graphicsDevice->createTexture("../assets/textures/hunter_normal.jpeg");
  auto hunter_roughness = graphicsDevice->createTexture("../assets/textures/hunter_roughness.jpeg");
  auto hunter_metallic = graphicsDevice->createTexture("../assets/textures/hunter_metallic.png");
  
  auto* pbr_mat = materialRegistry->getMaterial("pbr_mat");
  
  pbr_mat->setParameter("u_roughness",      0.15f); 
  pbr_mat->setParameter("u_specularTint",   1.0f);  
  pbr_mat->setParameter("u_specular",       0.0f);  
  pbr_mat->setParameter("u_anisotropic",    0.0f);  

  pbr_mat->setParameter("u_sheen",          0.0f);  
  pbr_mat->setParameter("u_sheenTint",      0.0f);

  pbr_mat->setParameter("u_clearcoat",      0.0f);  
  pbr_mat->setParameter("u_clearcoatGloss", 0.0f); 

  pbr_mat->setParameter("u_probeEnabled", 1);
  
  mam::Entity hunter1 = world->createEntity("HunterAXXX");
  world->addComponent<mam::RenderComponent>(hunter1, 
    mam::RenderComponent{
        .mesh = hunter_mesh.get(),
        .material = pbr_mat,
        .textures = {hunter_albedo.get(), nullptr, nullptr, nullptr}
    }
  );
  
  mam::Entity hunter2 = world->createEntity("HunterANXX");
  world->addComponent<mam::RenderComponent>(hunter2, 
    mam::RenderComponent{
        .mesh = hunter_mesh.get(),
        .material = pbr_mat,
        .textures = {hunter_albedo.get(), hunter_normal.get(), nullptr, nullptr}
    }
  );
  
  mam::Entity hunter3 = world->createEntity("HunterANRX");
  world->addComponent<mam::RenderComponent>(hunter3, 
    mam::RenderComponent{
        .mesh = hunter_mesh.get(),
        .material = pbr_mat,
        .textures = {hunter_albedo.get(), hunter_normal.get(), hunter_roughness.get(), nullptr}
    }
  );
  
  mam::Entity hunter4 = world->createEntity("HunterANXM");
  world->addComponent<mam::RenderComponent>(hunter4,
    mam::RenderComponent{
      .mesh = hunter_mesh.get(),
      .material = pbr_mat,
      .textures = {hunter_albedo.get(), hunter_normal.get(), hunter_roughness.get(), nullptr}
    }
  );
  
  mam::Entity hunter5 = world->createEntity("HunterANRM");
  world->addComponent<mam::RenderComponent>(hunter5,
    mam::RenderComponent{
      .mesh = hunter_mesh.get(),
      .material = pbr_mat,
      .textures = {hunter_albedo.get(), hunter_normal.get(), hunter_roughness.get(), nullptr}
    }
  );
  
  auto hunter1_tr = world->getComponent<mam::TransformComponent>(hunter1);
  hunter1_tr->position.x = -50.0f;
  hunter1_tr->rotation.x = -90.0f;
  hunter1_tr->rotation.z =  90.0f;
  
  auto hunter2_tr = world->getComponent<mam::TransformComponent>(hunter2);
  hunter2_tr->position.x = -25.0f;
  hunter2_tr->rotation.x = -90.0f;
  hunter2_tr->rotation.z =  90.0f;
  
  auto hunter3_tr = world->getComponent<mam::TransformComponent>(hunter3);
  hunter3_tr->position.x =  0.0f;
  hunter3_tr->rotation.x = -90.0f;
  hunter3_tr->rotation.z =  90.0f;
  
  auto hunter4_tr = world->getComponent<mam::TransformComponent>(hunter4);
  hunter4_tr->position.x =  25.0f;
  hunter4_tr->rotation.x = -90.0f;
  hunter4_tr->rotation.z =  90.0f;
  
  auto hunter5_tr = world->getComponent<mam::TransformComponent>(hunter5);
  hunter5_tr->position.x =  50.0f;
  hunter5_tr->rotation.x = -90.0f;
  hunter5_tr->rotation.z =  90.0f;
  
  mam::LightComponent lc = {
    .constant_att  = 1.0f,
    .ambient       = glm::vec3(0.15f, 0.15f, 0.15f),
    .linear_att    = 0.07f,
    .diffuse       = glm::vec3(12.0f, 12.0f, 12.0f),
    .quadratic_att = 0.017f,
    .specular      = glm::vec3(12.0f, 12.0f, 12.0f),
    .cut_off       = glm::cos(glm::radians(25.0f)),
    .outer_cut_off = glm::cos(glm::radians(35.0f)),
    .type          = mam::LightType::PointLight
  };
  
  mam::LightComponent dirLight = {
  .ambient = glm::vec3(0.03f),
  .diffuse = glm::vec3(0.25f),
  .specular = glm::vec3(0.1f),
  .direction = glm::vec3(-0.3f, -1.0f, -0.2f),
  .type = mam::LightType::DirectionalLight
  };

  auto lightEntity1 = world->createEntity("PointLight1");
  auto tr1 = world->getComponent<mam::TransformComponent>(lightEntity1);
  tr1->position = {-52.0f, 30.0f, 15.0f};
  world->addComponent<mam::LightComponent>(lightEntity1, lc);

  auto dirEntity = world->createEntity("DirectionalFill");
  world->addComponent<mam::LightComponent>(dirEntity, dirLight);

  mam::Entity probe1 = world->createEntity("LightProbe_Left");
  world->addComponent<mam::LightProbeComponent>(probe1, mam::LightProbeComponent{
    .influenceRadius = 65.0f, .blendDistance = 15.0f,
    .state = mam::ProbeState::NeedsBake, .bakePriority = 3
    });
  world->getComponent<mam::TransformComponent>(probe1)->position = { -40.0f, 5.0f, 0.0f };

  mam::Entity probe2 = world->createEntity("LightProbe_Center");
  world->addComponent<mam::LightProbeComponent>(probe2, mam::LightProbeComponent{
    .influenceRadius = 65.0f, .blendDistance = 15.0f,
    .state = mam::ProbeState::NeedsBake, .bakePriority = 2
    });
  world->getComponent<mam::TransformComponent>(probe2)->position = { 0.0f, 5.0f, 0.0f };

  mam::Entity probe3 = world->createEntity("LightProbe_Right");
  world->addComponent<mam::LightProbeComponent>(probe3, mam::LightProbeComponent{
    .influenceRadius = 65.0f, .blendDistance = 15.0f,
    .state = mam::ProbeState::NeedsBake, .bakePriority = 1
    });
  world->getComponent<mam::TransformComponent>(probe3)->position = { 40.0f, 5.0f, 0.0f };

  glm::vec3 target = world->getComponent<mam::TransformComponent>(hunter3)->position;
  
  glm::vec2 window_size = {
    static_cast<float>(window->getWidth()), 
    static_cast<float>(window->getHeight())
  };
  
  glm::vec3 initPosition = {0.0f, 30.0, 30.0f};
  
  camera->setPosition(initPosition);
  camera->initViewTarget(target, window_size);

  auto context = std::make_unique<mam::Context>();   
  context->Add<mam::MaterialRegistry>(materialRegistry);

  engine.update(*context);

  engine.cleanup();

  return 0;
}