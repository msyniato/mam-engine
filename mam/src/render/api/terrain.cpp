#include "render/api/terrain.hpp"
#include "render/api/vegetation.hpp"
#include "render/api/mesh.hpp"
#include "core/camera.hpp"
#include "core/input.hpp"
#include "ecs/system.hpp"
#include <imgui.h>
#include "ui/imgui_wrapper.hpp"
#include "render/api/river.hpp"
#include "render/api/graphics_device.hpp"
#include "ecs/world.hpp"
#include "ecs/engine_components.hpp"
#include "core/utils.hpp"
#include "Perlin/SimplexNoise.h"
#include "matsys/material.hpp" 
#include "jobsys/job_system.hpp"

namespace mam
{
  namespace {
    constexpr float kLodDist0   =  8000.f;
    constexpr float kLodDist1   = 12000.f;
    constexpr float kLodDist2   = 15000.f;
    constexpr float kLodDistMax =  1e10f;
  }

  float Terrain::sampleHeightRaw(float x, float z) const
  {
    if(!heightmapData_.empty())
    {
      float u = x / totalSizeX();
      float v = z / totalSizeZ();
      
			u = std::clamp(u, 0.0f, 1.0f);
      v = std::clamp(v, 0.0f, 1.0f);

			float px = u * (hmWidth_ - 1);
      float pz = v * (hmHeight_ - 1);

      int x0 = (int)px, x1 = std::min(x0 + 1, hmWidth_ - 1);
      int z0 = (int)pz, z1 = std::min(z0 + 1, hmHeight_ - 1);

      float fx = px - x0;
      float fz = pz - z0;

      float h00 = heightmapData_[z0 * hmWidth_ + x0] / 255.0f;
      float h10 = heightmapData_[z0 * hmWidth_ + x1] / 255.0f;
      float h01 = heightmapData_[z1 * hmWidth_ + x0] / 255.0f;
      float h11 = heightmapData_[z1 * hmWidth_ + x1] / 255.0f;

      float value = h00 * (1 - fx) * (1 - fz)
        + h10 * fx * (1 - fz)
        + h01 * (1 - fx) * fz
        + h11 * fx * fz;

      return value * noiseParams_.heightMult;
		}

    SimplexNoise noiseGen(
      noiseParams_.frequency,   
      1.0f,                     
      2.0f,                    
      noiseParams_.persistence  
    );

    float value = noiseGen.fractal(noiseParams_.octaves, x, z);

    value = (value + 1.0f) * 0.5f;

    value = std::powf(value, noiseParams_.redistribution);

    return value * noiseParams_.heightMult;
  }

  float Terrain::sampleHeight(float x, float z) const
  {
    if (heightField_.empty())
    {
      return sampleHeightRaw(x, z);
    }

    float u = x / totalSizeX();
    float v = z / totalSizeZ();
    u = std::clamp(u, 0.0f, 1.0f);
    v = std::clamp(v, 0.0f, 1.0f);

    float px = u * (hfWidth_ - 1);
    float pz = v * (hfHeight_ - 1);

    int x0 = (int)px, x1 = std::min(x0 + 1, hfWidth_ - 1);
    int z0 = (int)pz, z1 = std::min(z0 + 1, hfHeight_ - 1);

    float fx = px - x0;
    float fz = pz - z0;
    
    float h00 = heightField_[static_cast<size_t>(z0) * hfWidth_ + x0];
    float h10 = heightField_[static_cast<size_t>(z0) * hfWidth_ + x1];
    float h01 = heightField_[static_cast<size_t>(z1) * hfWidth_ + x0];
    float h11 = heightField_[static_cast<size_t>(z1) * hfWidth_ + x1];

    float value = h00 * (1 - fx) * (1 - fz)
                + h10 * fx * (1 - fz)
                + h01 * (1 - fx) * fz
                + h11 * fx * fz;

    return value;
  }

  float Terrain::riverMaskAt(float x, float z) const
  {
    const std::vector<float>& m = river_.mask();
    if (m.empty() || hfWidth_ <= 0 || hfHeight_ <= 0) return 0.0f;

    float u = std::clamp(x / totalSizeX(), 0.0f, 1.0f);
    float v = std::clamp(z / totalSizeZ(), 0.0f, 1.0f);
    int cx = int(u * (hfWidth_ - 1) + 0.5f);
    int cz = int(v * (hfHeight_ - 1) + 0.5f);
    return m[static_cast<size_t>(cz) * hfWidth_ + cx];
  }

  Terrain::Terrain(GraphicsDevice& gd,
                   const TerrainGridParams& grid,
                   const TerrainNoiseParams& noise,
                   const TerrainSplatParams& splat,
                   JobSystem* js)
  {
    init(gd, grid, noise, splat, js);
  }

  void Terrain::init(GraphicsDevice& gd,
    const TerrainGridParams& grid,
    const TerrainNoiseParams& noise,
    const TerrainSplatParams& splat,
    JobSystem* js)
  {
    gridParams_ = grid;
    noiseParams_ = noise;
    splatParams_ = splat;
    hmWidth_ = hmHeight_ = 0;

    rebuildHeightFieldAndWater(gd);

    buildAllChunks(gd, js);

    splatTextures_.clear();
    splatTextures_.push_back(gd.createTexture(splat.splatTexturePath0));
    splatTextures_.push_back(gd.createTexture(splat.splatTexturePath1));
    splatTextures_.push_back(gd.createTexture(splat.splatTexturePath2));
    splatTextures_.push_back(gd.createTexture(splat.detailTexturePath));
  }

  void Terrain::clearEntities(World& world)
  {
    for (Entity e : entities_)
    {
      if (e != kInvalidEntity)
        world.destroyEntity(e);
    }
    entities_.clear();

    if (waterEntity_ != kInvalidEntity)
    {
      world.destroyEntity(waterEntity_);
      waterEntity_ = kInvalidEntity;
    }

    chunks_.clear();
  }

  void Terrain::regenerate(GraphicsDevice& gd, World& world, Material* material, const TerrainGridParams& grid, const TerrainNoiseParams& noise, JobSystem* js)
  {
    clearEntities(world);

    gridParams_ = grid;
    noiseParams_ = noise;
    hmWidth_ = hmHeight_ = 0;

    rebuildHeightFieldAndWater(gd);
    buildAllChunks(gd, js);

    registerEntityInWorld(world, material);
    if (waterMaterial_ && waterMesh_)
      registerWaterInWorld(world, waterMaterial_, waterTextures_);
  }

  void Terrain::regenerate(GraphicsDevice& gd, World& world, Material* material, const TerrainNoiseParams& noise, JobSystem* js)
  {
    regenerate(gd, world, material, gridParams_, noise, js);
  }

  void Terrain::rebuildHeightFieldAndWater(GraphicsDevice& gd)
  {
    buildHeightField();

    glm::ivec2 src{ hfWidth_ / 5,        hfHeight_ / 5 };
    glm::ivec2 dst{ (hfWidth_ * 4) / 5,  (hfHeight_ * 4) / 5 };
    river_.build(heightField_, hfWidth_, hfHeight_, src, dst, hfCellSize_);

    waterMesh_.reset();
    if (!river_.empty()) {
      std::vector<Vertex>       wv;
      std::vector<unsigned int> wi;
      WaterMeshParams wp;
      buildWaterMesh(river_.path(), river_.bed(), hfCellSize_, wp, wv, wi);

      if (!wi.empty()) {
        waterMesh_ = std::make_unique<Mesh>(gd, "RiverWater", wv, wi);
        glm::vec3 mn(1e9f), mx(-1e9f);
        for (auto& v : wv) { mn = glm::min(mn, v.position); mx = glm::max(mx, v.position); }
        waterAABB_.min = mn;
        waterAABB_.max = mx;
      }
    }
  }

  void Terrain::registerEntityInWorld(World& world, Material* material)
  {
    entities_.clear();
    entities_.reserve(chunks_.size());

    for (const auto& chunk : chunks_)
    {
      Entity terrain_entity = world.createEntity("TerrainChunk");
      entities_.push_back(terrain_entity);

      std::vector<Texture*> texPtrs;
      for (auto& t : splatTextures_)
        if (t) texPtrs.push_back(t.get());

      glm::vec3 center = (chunk.aabb.min + chunk.aabb.max) * 0.5f;
      float radius = glm::length(chunk.aabb.max - center);

      RenderComponent rc;
      rc.mesh = chunk.mesh;
      rc.material = material;
      rc.textures = texPtrs;
      rc.aabbMin = chunk.aabb.min;
      rc.aabbMax = chunk.aabb.max;
      rc.boundingSphereCenter = center;
      rc.boundingSphereRadius = radius;
      rc.useSplatting = true;

      world.addComponent<RenderComponent>(terrain_entity, rc);

      auto* tc = world.getComponent<TransformComponent>(terrain_entity);
      if (tc) {
        tc->position = chunk.worldOrigin;
        tc->rotation = glm::vec3(0.f);
        tc->scale = glm::vec3(1.f);
      }

      LODComponent lod;
      for (int i = 0; i < 4; ++i)
        lod.meshes[i] = chunk.lodMeshes[i].get();
      lod.distances = { kLodDist0, kLodDist1, kLodDist2, kLodDistMax };
      lod.center = chunk.aabb.center();
      lod.levels = 4;
      world.addComponent<LODComponent>(terrain_entity, lod);
    }
  }

  void Terrain::registerWaterInWorld(World& world, Material* waterMat, const std::vector<Texture*>& textures)
  {
    waterMaterial_ = waterMat;
    waterTextures_ = textures;

    if (!waterMesh_) {
      waterEntity_ = kInvalidEntity;
      return;
    }

    Entity e = world.createEntity("RiverWater");
    waterEntity_ = e;

    glm::vec3 center = (waterAABB_.min + waterAABB_.max) * 0.5f;
    float radius = glm::length(waterAABB_.max - center);

    RenderComponent rc;
    rc.mesh = waterMesh_.get();
    rc.material = waterMat;
    rc.textures = textures;
    rc.aabbMin = waterAABB_.min;
    rc.aabbMax = waterAABB_.max;
    rc.boundingSphereCenter = center;
    rc.boundingSphereRadius = radius;
    world.addComponent<RenderComponent>(e, rc);

    auto* tc = world.getComponent<TransformComponent>(e);
    if (tc) {
      tc->position = glm::vec3(0.f);
      tc->rotation = glm::vec3(0.f);
      tc->scale = glm::vec3(1.f);
    }
  }

  void terrainBrushSystem(Context& ctx)
  {
    auto* brush = ctx.Get<TerrainBrushParams>();
    if (!brush || !brush->enabled) return;

    auto* input = ctx.Get<InputManager>();
    auto* camera = ctx.Get<Camera>();
    auto* terrain = ctx.Get<Terrain>();
    auto* world = ctx.Get<World>();
    auto* gd = ctx.Get<GraphicsDevice>();
    auto* ui = ctx.Get<ImGUIWrapper>();
    if (!input || !camera || !terrain || !world || !gd || !ui) return;

    const float dt = (float)world->lastDeltaTime;
    if (brush->cooldown > 0.f) brush->cooldown -= dt;

		ImGuiIO& io = ImGui::GetIO();
		const bool isUIFocused = io.WantCaptureMouse && !ui->getIsCameraMovable();
    if (isUIFocused) return;

    if (!input->isMouseButtonPressed(0)) return;
    if (brush->cooldown > 0.f) return;
    brush->cooldown = 1.f / 8.f; 

    glm::vec2 vp = ui->getViewportSize();
    glm::vec2 mouse = input->mousePosition();
    glm::vec2 vpMin = ui->getViewportMin();
    glm::vec2 localMouse = mouse - vpMin;

    if (vp.x <= 0.f || vp.y <= 0.f) return;
    if (localMouse.x < 0.f || localMouse.y < 0.f ||
      localMouse.x > vp.x || localMouse.y > vp.y) return;

    glm::vec3 origin, dir;
    Terrain::screenToWorldRay(localMouse, vp, *camera, origin, dir);

    glm::vec3 hit;
    if (!terrain->raycast(origin, dir, 50000.f, hit)) return;

    terrain->paintBrush(*gd, *world,
      hit.x, hit.z,
      brush->radius, brush->strength,
      brush->mode);

    auto* vegInstancer = ctx.Get<VegetationInstancer>();
    if (vegInstancer)
      vegInstancer->repositionOnTerrain(*gd, *terrain, *world);
  }
}