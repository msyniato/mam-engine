#include "render/api/terrain.hpp"
#include "render/api/vegetation.hpp"
#include "render/api/mesh.hpp"
#include "core/camera.hpp"
#include "core/input.hpp"
#include "ecs/system.hpp"
#include <imgui.h>
#include "ui/imgui_wrapper.hpp"
#include "render/api/graphics_device.hpp"
#include "ecs/world.hpp"
#include "ecs/engine_components.hpp"

namespace mam
{

  void Terrain::paintBrush(GraphicsDevice& gd, World& world, float worldX, float worldZ, float radius, float strength, TerrainBrushParams::Mode mode)
  {
    if (heightField_.empty() || hfCellSize_ <= 0.f) return;
    if (radius <= 0.f) return;

    const float cell = hfCellSize_;
    const int hw = hfWidth_;
    const int hh = hfHeight_;

    int x0 = std::max(0, (int)std::floor((worldX - radius) / cell));
    int x1 = std::min(hw - 1, (int)std::ceil((worldX + radius) / cell));
    int z0 = std::max(0, (int)std::floor((worldZ - radius) / cell));
    int z1 = std::min(hh - 1, (int)std::ceil((worldZ + radius) / cell));
    if (x0 > x1 || z0 > z1) return;

    const float sign = (mode == TerrainBrushParams::Mode::Raise) ? 1.f : -1.f;
    const float invR = 1.f / radius;

    for (int z = z0; z <= z1; ++z) {
      const float worldZ_ = z * cell;
      const float dz = worldZ_ - worldZ;
      for (int x = x0; x <= x1; ++x) {
        const float worldX_ = x * cell;
        const float dx = worldX_ - worldX;
        const float d = std::sqrt(dx * dx + dz * dz);
        if (d > radius) continue;

        float t = 1.f - d * invR;
        float fall = t * t * (3.f - 2.f * t);

        const size_t idx = (size_t)z * hw + x;
        float& h = heightField_[idx];

        switch (mode) {
        case TerrainBrushParams::Mode::Raise:
          h += strength * fall;
          break;

        case TerrainBrushParams::Mode::Lower:
          h -= strength * fall;
          break;

        case TerrainBrushParams::Mode::Smooth: {
          const int xm = std::max(0, x - 1);
          const int xp = std::min(hw - 1, x + 1);
          const int zm = std::max(0, z - 1);
          const int zp = std::min(hh - 1, z + 1);
          const float hL = heightField_[(size_t)z * hw + xm];
          const float hR = heightField_[(size_t)z * hw + xp];
          const float hU = heightField_[(size_t)zm * hw + x];
          const float hD = heightField_[(size_t)zp * hw + x];
          const float avg = (hL + hR + hU + hD) * 0.25f;
          float k = std::clamp(strength * 0.01f * fall, 0.f, 1.f);
          h += (avg - h) * k;
          break;
        }
        }
      }
    }

    const float chunkSize = gridParams_.chunkSizeM;
    const int   chunksX = (int)gridParams_.chunksX;
    const int   chunksZ = (int)gridParams_.chunksZ;

    int cx0 = std::max(0, (int)std::floor((worldX - radius) / chunkSize));
    int cx1 = std::min(chunksX - 1, (int)std::floor((worldX + radius) / chunkSize));
    int cz0 = std::max(0, (int)std::floor((worldZ - radius) / chunkSize));
    int cz1 = std::min(chunksZ - 1, (int)std::floor((worldZ + radius) / chunkSize));

    for (int cz = cz0; cz <= cz1; ++cz) {
      for (int cx = cx0; cx <= cx1; ++cx) {
        const u32 idx = (u32)cz * gridParams_.chunksX + (u32)cx;
        if (idx >= chunks_.size()) continue;

        ChunkCPUData cpu = buildChunkCPU((u32)cx, (u32)cz);
        chunks_[idx] = uploadChunkToGPU(gd, cpu);

        if (idx >= entities_.size()) continue;
        Entity e = entities_[idx];
        if (e == kInvalidEntity) continue;

        if (auto* rc = world.getComponent<RenderComponent>(e)) {
          rc->mesh = chunks_[idx].mesh;
          rc->aabbMin = chunks_[idx].aabb.min;
          rc->aabbMax = chunks_[idx].aabb.max;
          glm::vec3 c = (rc->aabbMin + rc->aabbMax) * 0.5f;
          rc->boundingSphereCenter = c;
          rc->boundingSphereRadius = glm::length(rc->aabbMax - c);
        }
        if (auto* lc = world.getComponent<LODComponent>(e)) {
          for (int i = 0; i < 4; ++i)
            lc->meshes[i] = chunks_[idx].lodMeshes[i].get();
          lc->center = chunks_[idx].aabb.center();
        }
      }
    }
  }

  void Terrain::screenToWorldRay(const glm::vec2& mousePixel, const glm::vec2& viewportSize, const Camera& camera, glm::vec3& outOrigin, glm::vec3& outDir)
  {
    if (viewportSize.x <= 0.f || viewportSize.y <= 0.f) {
      outOrigin = glm::vec3(0.f);
      outDir = glm::vec3(0.f, 0.f, -1.f);
      return;
    }

    const float ndcX = (2.f * mousePixel.x / viewportSize.x) - 1.f;
    const float ndcY = 1.f - (2.f * mousePixel.y / viewportSize.y);

    const glm::vec3 camPos   = camera.position();
    const glm::vec3 camFront = camera.front();
    const glm::vec3 camUp    = camera.up();

    const glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);
    const glm::mat4 proj = glm::perspective(glm::radians(camera.fov()),
      viewportSize.x / viewportSize.y,
      camera.zNear(), camera.zFar());
    const glm::mat4 invVP = glm::inverse(proj * view);

    glm::vec4 nearPt = invVP * glm::vec4(ndcX, ndcY, -1.f, 1.f);
    glm::vec4 farPt = invVP * glm::vec4(ndcX, ndcY, 1.f, 1.f);
    nearPt /= nearPt.w;
    farPt /= farPt.w;

    outOrigin = glm::vec3(nearPt);
    outDir = glm::normalize(glm::vec3(farPt - nearPt));
  }

  bool Terrain::raycast(const glm::vec3& origin, const glm::vec3& dir, float maxDist, glm::vec3& outHit) const
  {
    if (heightField_.empty()) return false;
    if (glm::length(dir) < 1e-6f) return false;

    const float step = 4.f;

    auto inBounds = [&](const glm::vec3& q) {
      return q.x >= -1.f && q.x <= totalSizeX() + 1.f &&
        q.z >= -1.f && q.z <= totalSizeZ() + 1.f;
      };
    auto signedAbove = [&](const glm::vec3& q) -> float {
      if (!inBounds(q)) return 1e9f;
      return q.y - sampleHeight(q.x, q.z);
      };

    glm::vec3 p = origin;
    glm::vec3 prevP = p;
    float prev = signedAbove(p);
    float traveled = 0.f;

    while (traveled < maxDist) {
      prevP = p;
      p += dir * step;
      traveled += step;

      const float cur = signedAbove(p);
      if (prev > 0.f && cur <= 0.f) {
        glm::vec3 a = prevP, b = p;
        for (int i = 0; i < 10; ++i) {
          glm::vec3 m = (a + b) * 0.5f;
          if (signedAbove(m) > 0.f) a = m;
          else b = m;
        }
        outHit = (a + b) * 0.5f;
        return true;
      }
      prev = cur;
    }
    return false;
  }

} // namespace mam
