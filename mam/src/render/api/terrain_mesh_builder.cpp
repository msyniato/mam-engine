#include "render/api/terrain.hpp"
#include "render/api/mesh.hpp"
#include "render/api/graphics_device.hpp"
#include "jobsys/job_system.hpp"

namespace mam
{

  void Terrain::buildHeightField()
  {
    const u32 subs = std::max(gridParams_.subdivisions, 1u);
		hfWidth_ = gridParams_.chunksX * subs + 1;
		hfHeight_ = gridParams_.chunksZ * subs + 1;
    hfCellSize_ = gridParams_.chunkSizeM / subs;

    heightField_.assign(static_cast<size_t>(hfWidth_) * hfHeight_, 0.0f);
    for (u32 z = 0; z < (u32)hfHeight_; ++z) {
      for (u32 x = 0; x < (u32)hfWidth_; ++x) {
        float wx = x * hfCellSize_;
        float wz = z * hfCellSize_;
        heightField_[static_cast<size_t>(z) * hfWidth_ + x] = sampleHeightRaw(wx, wz);
      }
    }
  }

  glm::vec3 Terrain::calcNormal(float x, float z, float step) const
  {
    float hL = sampleHeight(x - step, z);
    float hR = sampleHeight(x + step, z);
    float hD = sampleHeight(x, z - step);
    float hU = sampleHeight(x, z + step);

    glm::vec3 tangentX = glm::normalize(glm::vec3(2.0f * step, hR - hL, 0.0f));
    glm::vec3 tangentZ = glm::normalize(glm::vec3(0.0f, hU - hD, 2.0f * step));

    return glm::normalize(glm::cross(tangentZ, tangentX));
  }

  void Terrain::buildAllChunks(GraphicsDevice& gd, JobSystem* js)
  {
    const u32 chunksX = gridParams_.chunksX;
    const u32 chunksZ = gridParams_.chunksZ;
    const size_t totalChunks = static_cast<size_t>(chunksX) * chunksZ;

    chunks_.clear();
    chunks_.resize(totalChunks);

    if (js) {
      std::vector<ChunkCPUData> cpuData(totalChunks);
      std::vector<JobHandle> handles;

      for (u32 cz = 0; cz < chunksZ; ++cz) {
        for (u32 cx = 0; cx < chunksX; ++cx) {
          u32 idx = cz * chunksX + cx;
          handles.push_back(js->submit_callable([this, &cpuData, idx, cx, cz]() {
            cpuData[idx] = buildChunkCPU(cx, cz);
            }));
        }
      }

      js->wait_all(handles);

      for (size_t i = 0; i < totalChunks; ++i){
        chunks_[i] = uploadChunkToGPU(gd, cpuData[i]);
      }
    }
    else {
      for (u32 cz = 0; cz < chunksZ; ++cz) {
        for (u32 cx = 0; cx < chunksX; ++cx) {
          u32 idx = cz * chunksX + cx;
          auto cpu = buildChunkCPU(cx, cz);
          chunks_[idx] = uploadChunkToGPU(gd, cpu);
        }
      }
    }
  }

  std::unique_ptr<Mesh> Terrain::buildLODMesh(GraphicsDevice& gd, u32 cx, u32 cz, u32 subs) const
  {
    const float chunkSize = gridParams_.chunkSizeM;
    const float cellSize = chunkSize / subs;
    const u32   vertPerSide = subs + 1;
    const float originX = cx * chunkSize;
    const float originZ = cz * chunkSize;

    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(static_cast<size_t>(vertPerSide) * vertPerSide);
    indices.reserve(static_cast<size_t>(subs) * subs * 6);

    for (u32 iz = 0; iz < vertPerSide; ++iz) {
      for (u32 ix = 0; ix < vertPerSide; ++ix) {
        const float wx = originX + ix * cellSize;
        const float wz = originZ + iz * cellSize;
        const float wy = sampleHeight(wx, wz);

        Vertex v;
        v.position = glm::vec3(wx, wy, wz);
        v.normal = calcNormal(wx, wz, cellSize);
        v.texCoord = glm::vec2(
          wx / totalSizeX(),
          wz / totalSizeZ()
        );
        vertices.push_back(v);
      }
    }

    for (u32 iz = 0; iz < subs; ++iz) {
      for (u32 ix = 0; ix < subs; ++ix) {
        const u32 tl = iz * vertPerSide + ix;
        const u32 bl = (iz + 1) * vertPerSide + ix;
        const u32 br = (iz + 1) * vertPerSide + (ix + 1);
        const u32 tr = iz * vertPerSide + (ix + 1);

        indices.push_back(tl); indices.push_back(bl); indices.push_back(br);
        indices.push_back(tl); indices.push_back(br); indices.push_back(tr);
      }
    }

    const std::string name = "Chunk_" + std::to_string(cx) + "_"
      + std::to_string(cz) + "_LOD"
      + std::to_string(gridParams_.subdivisions / subs);
    return std::make_unique<Mesh>(gd, name, vertices, indices);
  }

  Terrain::ChunkCPUData Terrain::buildChunkCPU(u32 cx, u32 cz) const
  {
    ChunkCPUData data;
    data.cx = cx;
    data.cz = cz;

    const float chunkSize = gridParams_.chunkSizeM;
    const u32   baseSubs = gridParams_.subdivisions;
    const float originX = cx * chunkSize;
    const float originZ = cz * chunkSize;

    data.worldOrigin = glm::vec3(originX, 0.f, originZ);

    const std::array<u32, 4> lodSubs = {
        std::max(baseSubs, 1u),
        std::max(baseSubs / 2u, 2u),
        std::max(baseSubs / 4u, 2u),
        std::max(baseSubs / 8u, 2u)
    };

    float minY = 1e9f, maxY = -1e9f;

    for (int lod = 0; lod < 4; ++lod) {
      u32 subs = lodSubs[lod];
      const float cellSize = chunkSize / static_cast<float>(subs);
      const u32   vertPerSide = subs + 1;

      auto& verts = data.lodVertices[lod];
      auto& indices = data.lodIndices[lod];
      verts.reserve(static_cast<size_t>(vertPerSide) * vertPerSide);
      indices.reserve(static_cast<size_t>(subs) * subs * 6);

      for (u32 iz = 0; iz < vertPerSide; ++iz) {
        for (u32 ix = 0; ix < vertPerSide; ++ix) {
          const float wx = originX + ix * cellSize;
          const float wz = originZ + iz * cellSize;
          const float wy = sampleHeight(wx, wz);

          if (lod == 0) {
            minY = std::min(minY, wy);
            maxY = std::max(maxY, wy);
          }

          Vertex v;
          v.position = glm::vec3(ix * cellSize, wy, iz * cellSize);
          v.normal = calcNormal(wx, wz, cellSize);
          v.texCoord = glm::vec2(wx / totalSizeX(), wz / totalSizeZ());
          verts.push_back(v);
        }
      }

      for (u32 iz = 0; iz < subs; ++iz) {
        for (u32 ix = 0; ix < subs; ++ix) {
          const u32 tl = iz * vertPerSide + ix;
          const u32 bl = (iz + 1) * vertPerSide + ix;
          const u32 br = (iz + 1) * vertPerSide + (ix + 1);
          const u32 tr = iz * vertPerSide + (ix + 1);
          indices.push_back(tl); indices.push_back(bl); indices.push_back(br);
          indices.push_back(tl); indices.push_back(br); indices.push_back(tr);
        }
      }
      const float skirtDepth = noiseParams_.heightMult * 0.5f;
      const u32 skirtBase = static_cast<u32>(verts.size());

      for (u32 ix = 0; ix < vertPerSide; ++ix) {
        Vertex sv = verts[0 * vertPerSide + ix];
        sv.position.y -= skirtDepth;
        verts.push_back(sv);
      }
      for (u32 ix = 0; ix < vertPerSide; ++ix) {
        Vertex sv = verts[subs * vertPerSide + ix];
        sv.position.y -= skirtDepth;
        verts.push_back(sv);
      }
      for (u32 iz = 0; iz < vertPerSide; ++iz) {
        Vertex sv = verts[iz * vertPerSide + 0];
        sv.position.y -= skirtDepth;
        verts.push_back(sv);
      }
      for (u32 iz = 0; iz < vertPerSide; ++iz) {
        Vertex sv = verts[iz * vertPerSide + subs];
        sv.position.y -= skirtDepth;
        verts.push_back(sv);
      }
      for (u32 ix = 0; ix < subs; ++ix) {
        u32 t0 = 0 * vertPerSide + ix, t1 = 0 * vertPerSide + ix + 1;
        u32 b0 = skirtBase + ix, b1 = skirtBase + ix + 1;
        indices.push_back(t0); indices.push_back(b1); indices.push_back(b0);
        indices.push_back(t0); indices.push_back(t1); indices.push_back(b1);
      }
      for (u32 ix = 0; ix < subs; ++ix) {
        u32 t0 = subs * vertPerSide + ix, t1 = subs * vertPerSide + ix + 1;
        u32 b0 = skirtBase + vertPerSide + ix, b1 = skirtBase + vertPerSide + ix + 1;
        indices.push_back(t0); indices.push_back(b0); indices.push_back(b1);
        indices.push_back(t0); indices.push_back(b1); indices.push_back(t1);
      }
      for (u32 iz = 0; iz < subs; ++iz) {
        u32 t0 = iz * vertPerSide + 0, t1 = (iz + 1) * vertPerSide + 0;
        u32 b0 = skirtBase + 2 * vertPerSide + iz, b1 = skirtBase + 2 * vertPerSide + iz + 1;
        indices.push_back(t0); indices.push_back(b0); indices.push_back(b1);
        indices.push_back(t0); indices.push_back(b1); indices.push_back(t1);
      }
      for (u32 iz = 0; iz < subs; ++iz) {
        u32 t0 = iz * vertPerSide + subs, t1 = (iz + 1) * vertPerSide + subs;
        u32 b0 = skirtBase + 3 * vertPerSide + iz, b1 = skirtBase + 3 * vertPerSide + iz + 1;
        indices.push_back(t0); indices.push_back(b1); indices.push_back(b0);
        indices.push_back(t0); indices.push_back(t1); indices.push_back(b1);
      }
    }

    data.aabbMin = glm::vec3(originX, minY, originZ);
    data.aabbMax = glm::vec3(originX + chunkSize, maxY, originZ + chunkSize);
    return data;
  }

  TerrainChunk Terrain::uploadChunkToGPU(GraphicsDevice& gd, const ChunkCPUData& cpuData) const
  {
    const std::array<u32, 4> lodSubs = {
        gridParams_.subdivisions,
        std::max(gridParams_.subdivisions / 2u, 2u),
        std::max(gridParams_.subdivisions / 4u, 2u),
        std::max(gridParams_.subdivisions / 8u, 2u)
    };

    TerrainChunk chunk;
    chunk.cx = cpuData.cx;
    chunk.cz = cpuData.cz;
    chunk.worldOrigin = cpuData.worldOrigin;
    chunk.aabb.min = cpuData.aabbMin;
    chunk.aabb.max = cpuData.aabbMax;

    for (int i = 0; i < 4; ++i) {
      const std::string name = "Chunk_" + std::to_string(cpuData.cx) + "_"
        + std::to_string(cpuData.cz) + "_LOD" + std::to_string(i);
      chunk.lodMeshes[i] = std::make_unique<Mesh>(gd, name,
        cpuData.lodVertices[i], cpuData.lodIndices[i]);
    }
    chunk.mesh = chunk.lodMeshes[0].get();
    return chunk;
  }

  TerrainChunk Terrain::buildChunk(GraphicsDevice& gd, u32 cx, u32 cz) const
  {
    const float chunkSize = gridParams_.chunkSizeM;
    const u32   baseSubs = gridParams_.subdivisions;
    const float originX = cx * chunkSize;
    const float originZ = cz * chunkSize;

    const u32   vertPerSide = baseSubs + 1;
    const float cellSize = chunkSize / baseSubs;
    float minY = 1e9f, maxY = -1e9f;
    for (u32 iz = 0; iz < vertPerSide; ++iz)
      for (u32 ix = 0; ix < vertPerSide; ++ix) {
        float wy = sampleHeight(originX + ix * cellSize, originZ + iz * cellSize);
        minY = std::min(minY, wy);
        maxY = std::max(maxY, wy);
      }

    const std::array<u32, 4> lodSubs = {
        baseSubs,
        std::max(baseSubs / 2u, 2u),
        std::max(baseSubs / 4u, 2u),
        std::max(baseSubs / 8u, 2u)
    };

    TerrainChunk chunk;
    chunk.cx = cx;
    chunk.cz = cz;
    chunk.worldOrigin = glm::vec3(originX, 0.f, originZ);
    chunk.aabb.min = glm::vec3(originX, minY, originZ);
    chunk.aabb.max = glm::vec3(originX + chunkSize, maxY, originZ + chunkSize);

    for (int i = 0; i < 4; ++i)
      chunk.lodMeshes[i] = buildLODMesh(gd, cx, cz, lodSubs[i]);

    chunk.mesh = chunk.lodMeshes[0].get();
    return chunk;
  }

} // namespace mam
