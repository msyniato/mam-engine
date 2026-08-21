#include "mam/engine.hpp"

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
  // Tight shadow frustum: the scene is only ~10 units wide, so ±15 concentrates
  // all 2048 shadow-map texels over that area instead of the default ±512.
  engine.getRenderer()->setShadowOrthoExtent(15.f);
  
  std::array<std::string, 6> skyboxFaces = {
    "../assets/textures/skybox/right.jpg",   //derecha
    "../assets/textures/skybox/left.jpg",   //izquierda
    "../assets/textures/skybox/top.jpg",   //arriba
    "../assets/textures/skybox/bottom.jpg",   //abajo
    "../assets/textures/skybox/front.jpg",   //frente
    "../assets/textures/skybox/back.jpg",   //atras
  };
  auto skyboxCubemap = graphicsDevice->createCubemap(skyboxFaces);
  
  auto* skyboxMat = materialRegistry->getMaterial("skybox_mat");
  engine.getRenderer()->setSkybox(skyboxCubemap.get(), skyboxMat);
  
  std::unique_ptr<mam::Mesh> hunter_mesh = std::make_unique<mam::Mesh>(*graphicsDevice, "hunter_mesh");
  hunter_mesh->LoadOBJ("../assets/models/hunter.obj");

  
  std::unique_ptr<mam::Mesh> cube_mesh = std::make_unique<mam::Mesh>(*graphicsDevice, "cube_mesh");
  cube_mesh->createCube24v();
  
  auto hunter_albedo = graphicsDevice->createTexture("../assets/textures/hunter_albedo.jpeg");
  auto hunter_normal = graphicsDevice->createTexture("../assets/textures/hunter_normal.jpeg");
  auto hunter_roughness = graphicsDevice->createTexture("../assets/textures/hunter_roughness.jpeg");
  auto hunter_metallic = graphicsDevice->createTexture("../assets/textures/hunter_metallic.png");
  auto ground = graphicsDevice->createTexture("../assets/textures/grey.jpg");
  
  auto* pbr_mat = materialRegistry->getMaterial("pbr_mat");

  pbr_mat->setParameter("u_roughness", 0.15f);
  pbr_mat->setParameter("u_specularTint", 1.0f);
  pbr_mat->setParameter("u_specular", 0.0f);
  pbr_mat->setParameter("u_anisotropic", 0.0f);

  pbr_mat->setParameter("u_sheen", 0.0f);
  pbr_mat->setParameter("u_sheenTint", 0.0f);

  pbr_mat->setParameter("u_clearcoat",      0.0f);  
  pbr_mat->setParameter("u_clearcoatGloss", 0.0f); 
  
  mam::Entity hunter = world->createEntity("Hunter");
  world->addComponent<mam::RenderComponent>(hunter, 
    mam::RenderComponent{
        .mesh = hunter_mesh.get(),
        .material = pbr_mat,
        .textures = {hunter_albedo.get(), hunter_normal.get(), hunter_roughness.get(), nullptr}
    }
  );
  
  mam::Entity plane = world->createEntity("Plane");
  world->addComponent<mam::RenderComponent>(plane, 
    mam::RenderComponent{
        .mesh = cube_mesh.get(),
        .material = pbr_mat,
        .textures = {ground.get(), nullptr, nullptr, nullptr}
    }
  );
  
  auto hunter_tr = world->getComponent<mam::TransformComponent>(hunter);
  hunter_tr->rotation.x = -90.0f;
  hunter_tr->rotation.y = -0.5f;
  hunter_tr->scale = {0.25f, 0.25f, 0.25f};
  
  auto plane_tr = world->getComponent<mam::TransformComponent>(plane);
  plane_tr->position.y =  0.375f;
  plane_tr->scale =  {10.0f, 0.05f, 10.0f};
  
  mam::LightComponent lc = {
    .constant_att  = 1.0f,
    .ambient       = glm::vec3(0.03f, 0.03f, 0.03f),  
    .linear_att    = 0.07f,
    .diffuse       = glm::vec3(6.0f, 6.0f, 6.0f),      
    .quadratic_att = 0.017f,
    .specular      = glm::vec3(6.0f, 6.0f, 6.0f),      
    .cut_off       = glm::cos(glm::radians(25.0f)),
    .outer_cut_off = glm::cos(glm::radians(35.0f)),
    .type          = mam::LightType::PointLight
  };
  
  /*mam::LightComponent sunLight = {
    
    .constant_att  = 1.0f,
    
    .ambient = glm::vec3(0.05f),
    .linear_att    = 0.0f,
    
    .diffuse = glm::vec3(4.0f),
    .quadratic_att = 0.0f,
    
    .specular = glm::vec3(4.0f),
    .cut_off = 0.0f,
    
    .direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f)),
    .outer_cut_off = 0.0f,

    .type = mam::LightType::DirectionalLight
  };*/
  
  auto sun = world->createEntity("Sun");
  auto tr1 = world->getComponent<mam::TransformComponent>(sun);
  tr1->position = {-5.0f, 10.0f, 5.0f};
  world->addComponent<mam::LightComponent>(sun, lc);

  mam::LightComponent sunDirLC{};
  sunDirLC.type      = mam::LightType::DirectionalLight;
  sunDirLC.direction = glm::normalize(glm::vec3(0.5f, -1.0f, 0.3f));
  sunDirLC.ambient   = glm::vec3(0.05f, 0.05f, 0.06f);
  sunDirLC.diffuse   = glm::vec3(2.5f, 2.3f, 2.0f);
  sunDirLC.specular  = glm::vec3(2.5f, 2.3f, 2.0f);
  auto sunDirEntity  = world->createEntity("DirectionalSun");
  world->addComponent<mam::LightComponent>(sunDirEntity, sunDirLC);
  
  glm::vec3 target = world->getComponent<mam::TransformComponent>(hunter)->position;
  
  glm::vec2 window_size = {
    static_cast<float>(window->getWidth()), 
    static_cast<float>(window->getHeight())
  };
  
  glm::vec3 initPosition = {-5.0f, 5.0, 5.0f};
  
  camera->setPosition(initPosition);
  camera->initViewTarget(target, window_size);

  auto context = std::make_unique<mam::Context>();
  engine.update(*context);

  engine.cleanup();

  return 0;
}