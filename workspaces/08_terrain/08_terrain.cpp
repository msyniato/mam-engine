#include "mam/engine.hpp"
#include "mam/core/input.hpp"

constexpr u32 w = 1280;
constexpr u32 h = 720;

int main(int argc, char** argv)
{
  mam::GraphicsContext::Desc desc = {
        w, h,
        mam::GraphicsContext::RenderAPI::OpenGL
  };

  mam::Engine engine(desc);
  engine.init();

  auto worldRegistry = engine.getWorldRegistry();
  auto materialRegistry = engine.getMaterialRegistry();
  auto window = engine.getWindow();
  auto graphicsDevice = engine.getGraphicsContext()->getGraphicsDevice();
  auto camera = engine.getCamera();
  auto renderer = engine.getRenderer();
	auto input = engine.getInputManager();
  auto world = worldRegistry->getCurrentWorld();
  auto scene = world->getCurrentScene();  
	auto ui = engine.getImGUIWrapper();

  bool ssaoEnabled = true;
  renderer->setSSAOEnabled(ssaoEnabled);

  mam::TerrainGridParams grid;
  grid.chunksX = 16;
  grid.chunksZ = 16;
  grid.chunkSizeM = 1000.f;
  grid.subdivisions = 64;

  mam::TerrainNoiseParams noise;
  noise.frequency = 0.0008f;
  noise.heightMult = 500.f;
  noise.octaves = 7;
  noise.persistence = 0.45f;
  noise.redistribution = 1.6f;
  noise.heightmapPath = "../assets/textures/testheightmap.jpg";  

  mam::TerrainSplatParams splat;
  splat.splatTexturePath0 = "../assets/textures/grass.jpg";
  splat.splatTexturePath1 = "../assets/textures/ground2.jpg";
  splat.splatTexturePath2 = "../assets/textures/snow.jpg";
	splat.detailTexturePath = "../assets/textures/testheightmap.jpg";
  
  std::array<std::string, 6> skyboxFaces = {
    "../assets/textures/skybox/right.jpg",   //derecha
    "../assets/textures/skybox/left.jpg",   //izquierda
    "../assets/textures/skybox/top.jpg",   //arriba
    "../assets/textures/skybox/bottom.jpg",   //abajo
    "../assets/textures/skybox/front.jpg",   //frente
    "../assets/textures/skybox/back.jpg",   //atras
  };
  auto skyboxCubemap = graphicsDevice->createCubemap(skyboxFaces);

  auto* terrainMat = materialRegistry->getMaterial("terrain_mat");
  auto* vegMat = materialRegistry->getMaterial("vegetation_mat");
	auto* riverMat = materialRegistry->getMaterial("basic_mat");
  auto riverTex = graphicsDevice->createTexture("../assets/textures/lol.png");
  auto* skyboxMat = materialRegistry->getMaterial("skybox_mat");

  mam::Terrain terrain(*graphicsDevice, grid, noise, splat, engine.getJobSystem());
  terrain.registerEntityInWorld(*world, terrainMat);
  terrain.registerWaterInWorld(*world, riverMat, { riverTex.get() });
  engine.getRenderer()->setSkybox(skyboxCubemap.get(), skyboxMat);

  mam::LSystemParams ls;
	ls.axiom = "X";
  ls.rules = {
    {'X', "F[&+X][&-X]F[/+X]X"},
    {'F', "FF"}
	};
	ls.angle = 25.f;
	ls.iterations = 4;
	ls.segmentLength = 1.6f;
	ls.segmentRadius = 0.5f;
	ls.lengthDecay = 0.72f;
	ls.radiusDecay = 1.0f;
	ls.cylinderSides = 5;

  mam::VegetationParams vegParams;
  vegParams.count = 5000;
  vegParams.minScale = 1.f;
  vegParams.maxScale = 3.f;
  vegParams.minHeightToSpawn = 5.f;
  vegParams.maxHeightToSpawn = 250.f;
  vegParams.seed = 42;

	mam::VegetationInstancer vegInstancer(*graphicsDevice, terrain, ls, vegParams);
	vegInstancer.registerEntitiesInWorld(*world, vegMat);

  // Directional light (sol) — registrado en el ECS para que collectLightsSystem lo recoja cada frame
  mam::LightComponent sunLC{};
  sunLC.type      = mam::LightType::DirectionalLight;
  sunLC.direction = glm::normalize(glm::vec3(0.4f, -1.0f, 0.3f));
  sunLC.ambient   = glm::vec3(0.05f, 0.05f, 0.06f);
  sunLC.diffuse   = glm::vec3(4.0f, 3.8f, 3.5f);
  sunLC.specular  = glm::vec3(4.0f, 3.8f, 3.5f);

  auto sunEntity  = world->createEntity("Sun");
  world->addComponent<mam::LightComponent>(sunEntity, sunLC);

  glm::vec3 center = terrain.getPosition();
  camera->setPosition(glm::vec3(center.x, 300.f, center.z - 500.f));
  camera->initViewTarget(glm::vec3(center.x, 100.f, center.z), { w, h });
	
  mam::WorldGenParams initialParams;
  initialParams.grid       = grid;
  initialParams.noise      = noise;
  initialParams.splat      = splat;
  initialParams.lsystem    = ls;
  initialParams.vegetation = vegParams;
  ui->setWorldGenParams(initialParams);

	ui->setWorldGenCallback([&](const mam::WorldGenParams& p)
	{
		vegInstancer.clear(*world);
		terrain.regenerate(*graphicsDevice, *worldRegistry->getCurrentWorld(), terrainMat, p.grid, p.noise, nullptr);
		vegInstancer = mam::VegetationInstancer(*graphicsDevice, terrain, p.lsystem, p.vegetation);
		vegInstancer.registerEntitiesInWorld(*world, vegMat);
	});

  mam::TerrainBrushParams brush;
  brush.radius = 200.f;
  brush.strength = 25.f;
  brush.mode = mam::TerrainBrushParams::Mode::Raise;
  brush.enabled = false;

  ui->setExtraImGuiCallback([&](mam::Context& ctx) {
    ImGui::Begin("Render");

    bool isDeferred = renderer->isDeferred_;
    if (ImGui::Checkbox("Deferred Rendering", &isDeferred))
      renderer->setDeferredEnabled(isDeferred);

    ImGui::Separator();

    if (renderer->isDeferred_) {
      if (ImGui::Checkbox("SSAO", &ssaoEnabled))
        renderer->setSSAOEnabled(ssaoEnabled);
      ImGui::Text("SSAO pass: %.2f ms", renderer->getSSAOPassMs());
    }

    ImGui::End();

    ImGui::Begin("Terrain Brush");
    ImGui::Checkbox("Enabled (LMB to paint)", &brush.enabled);
    ImGui::SliderFloat("Radius (m)", &brush.radius, 20.f, 800.f, "%.0f");
    ImGui::SliderFloat("Strength (m)", &brush.strength, 0.5f, 80.f, "%.1f");

    int modeIdx = (int)brush.mode;
    if (ImGui::RadioButton("Raise", modeIdx == 0)) brush.mode = mam::TerrainBrushParams::Mode::Raise;
    ImGui::SameLine();
    if (ImGui::RadioButton("Lower", modeIdx == 1)) brush.mode = mam::TerrainBrushParams::Mode::Lower;
    ImGui::SameLine();
    if (ImGui::RadioButton("Smooth", modeIdx == 2)) brush.mode = mam::TerrainBrushParams::Mode::Smooth;

    ImGui::TextDisabled("Click and drag on the terrain.");
    ImGui::End();
    });

  mam::Signature emptySig;
  auto* sys = world->registerSystem("terrain_brush_system", emptySig);
  sys->onUpdate = mam::terrainBrushSystem;

  auto context = std::make_unique<mam::Context>();
  context->Add<mam::Terrain>(&terrain);
	context->Add<mam::VegetationInstancer>(&vegInstancer);
  context->Add<mam::TerrainBrushParams>(&brush);
  context->Add<mam::GraphicsDevice>(graphicsDevice);
  context->Add<mam::MaterialRegistry>(materialRegistry);
  context->Add<mam::ImGUIWrapper>(ui);

  engine.update(*context);
  engine.cleanup();

  return 0;
}