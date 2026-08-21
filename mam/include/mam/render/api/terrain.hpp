#pragma once

#include "common/common.hpp"
#include "ecs/entity.hpp"
#include "render/api/river.hpp"

namespace mam {

  class GraphicsDevice;
  class Mesh;
  class World;
  struct Scene;
  class Texture;
  class Material;
  class JobSystem;
  class Camera;
  class Context;

  /**
   * @brief Parameters for procedural terrain height generation.
   */
  struct TerrainNoiseParams {
    float       frequency      = 0.015f; ///< Base noise frequency.
    float       heightMult     = 40.0f;  ///< Global height scale multiplier.
    float       persistence    = 0.5f;   ///< Amplitude decay factor per octave.
    float       redistribution = 1.4f;   ///< Height redistribution exponent.
    int         octaves        = 6;      ///< Number of noise octaves.
    std::string heightmapPath  = "";     ///< Optional path to a heightmap image; overrides procedural noise when non-empty.
  };

  /**
   * @brief Parameters defining the terrain chunk grid layout.
   */
  struct TerrainGridParams {
    u32   chunksX      = 16;   ///< Number of chunks along the X axis.
    u32   chunksZ      = 16;   ///< Number of chunks along the Z axis.
    float chunkSizeM   = 64.f; ///< World-space size of each chunk (meters).
    u32   subdivisions = 0;    ///< Additional tessellation subdivisions per chunk.
  };

  /**
   * @brief Axis-aligned bounding box for a terrain chunk.
   */
  struct TerrainAABB {
    glm::vec3 min; ///< Lower corner of the AABB.
    glm::vec3 max; ///< Upper corner of the AABB.

    /// Returns the centre point of the AABB.
    glm::vec3 center() const { return (min + max) * 0.5f; }
  };

  /**
   * @brief Texture paths used for terrain splat-map rendering.
   */
  struct TerrainSplatParams {
    std::string splatTexturePath0 = "../assets/textures/grass.jpg";       ///< Low-altitude texture.
    std::string splatTexturePath1 = "../assets/textures/rock.jpg";        ///< Mid-altitude texture.
    std::string splatTexturePath2 = "../assets/textures/snow.jpg";        ///< High-altitude texture.
    std::string detailTexturePath = "../assets/textures/testheightmap.jpg"; ///< Detail/noise texture.
  };

  /**
   * @brief Parameters for the interactive terrain sculpting brush.
   */
  struct TerrainBrushParams {
    /**
     * @brief Available sculpting modes.
     */
    enum class Mode { Raise = 0, Lower = 1, Smooth = 2 };

    Mode  mode     = Mode::Raise; ///< Active sculpt mode.
    float radius   = 200.0f;      ///< Brush radius in world units.
    float strength = 25.0f;       ///< Brush strength multiplier.
    bool  enabled  = false;       ///< Whether the brush is currently active.
    float cooldown = 0.1f;        ///< Minimum time between applications (seconds).
  };

  /**
   * @brief Represents a single terrain chunk with its mesh and LOD variants.
   */
  struct TerrainChunk {
    Mesh*                                mesh = nullptr; ///< Active mesh pointer (aliases lodMeshes[0]).
    std::array<std::unique_ptr<Mesh>, 4> lodMeshes;      ///< LOD mesh variants (0 = highest detail).
    TerrainAABB                          aabb;            ///< World-space AABB for frustum culling.
    glm::vec3                            worldOrigin;     ///< World-space origin of this chunk.
    u32                                  cx = 0;          ///< Chunk column index.
    u32                                  cz = 0;          ///< Chunk row index.
  };

  /**
   * @brief Chunked, LOD-aware terrain with procedural height generation and river carving.
   *
   * The terrain is subdivided into a uniform grid of chunks, each with up to four LOD mesh
   * variants. Height data is generated from fractional Brownian motion noise or an optional
   * heightmap image, and the surface is optionally carved by the River subsystem.
   */
  class Terrain {
  public:
    /**
     * @brief Construct and fully initialise the terrain.
     * @param gd    Graphics device for GPU resource creation.
     * @param grid  Chunk grid layout parameters.
     * @param noise Height noise parameters.
     * @param splat Splat texture paths.
     * @param js    Job system for parallel chunk generation (may be nullptr).
     */
    Terrain(GraphicsDevice& gd,
            const TerrainGridParams& grid,
            const TerrainNoiseParams& noise,
            const TerrainSplatParams& splat,
            JobSystem* js);
    ~Terrain() = default;

    /**
     * @brief Initialise or re-initialise all terrain data and GPU resources.
     * @param gd    Graphics device.
     * @param grid  Grid parameters.
     * @param noise Noise parameters.
     * @param splat Splat parameters.
     * @param js    Job system.
     */
    void init(GraphicsDevice& gd,
              const TerrainGridParams& grid,
              const TerrainNoiseParams& noise,
              const TerrainSplatParams& splat,
              JobSystem* js);

    /// Returns all terrain chunks.
    const std::vector<TerrainChunk>& getChunks()     const { return chunks_; }
    /// Returns the current grid parameters.
    const TerrainGridParams&  getGridParams()         const { return gridParams_; }
    /// Returns the current noise parameters.
    const TerrainNoiseParams& getNoiseParams()        const { return noiseParams_; }
    /// Returns the terrain's world-space centre position.
    glm::vec3 getPosition()                           const { return { totalSizeX() * 0.5f, 0.f, totalSizeZ() * 0.5f }; }
    /// Returns a mutable reference to the noise parameters.
    TerrainNoiseParams& getNoiseParamsMutable()             { return noiseParams_; }
    /// Returns a mutable reference to the grid parameters.
    TerrainGridParams&  getGridParamsMutable()              { return gridParams_; }

    /**
     * @brief Remove all terrain ECS entities from the world.
     * @param world ECS world owning the entities.
     */
    void clearEntities(World& world);

    /**
     * @brief Fully regenerate the terrain with new grid and noise parameters.
     * @param gd       Graphics device.
     * @param world    ECS world.
     * @param material Terrain render material.
     * @param grid     New grid parameters.
     * @param noise    New noise parameters.
     * @param js       Job system.
     */
    void regenerate(GraphicsDevice& gd, World& world, Material* material,
                    const TerrainGridParams& grid, const TerrainNoiseParams& noise,
                    JobSystem* js);

    /**
     * @brief Regenerate the terrain keeping the existing grid, with new noise parameters only.
     * @param gd       Graphics device.
     * @param world    ECS world.
     * @param material Terrain render material.
     * @param noise    New noise parameters.
     * @param js       Job system.
     */
    void regenerate(GraphicsDevice& gd, World& world, Material* material,
                    const TerrainNoiseParams& noise, JobSystem* js);

    /**
     * @brief Apply a sculpting brush at a world-space XZ position.
     * @param gd       Graphics device.
     * @param world    ECS world.
     * @param worldX   Brush centre X in world space.
     * @param worldZ   Brush centre Z in world space.
     * @param radius   Brush radius in world units.
     * @param strength Brush strength.
     * @param mode     Sculpt mode (Raise / Lower / Smooth).
     */
    void paintBrush(GraphicsDevice& gd, World& world,
                    float worldX, float worldZ,
                    float radius, float strength,
                    TerrainBrushParams::Mode mode);

    /**
     * @brief Unproject a screen pixel to a world-space ray.
     * @param mousePixel   Screen-space pixel coordinate.
     * @param viewportSize Viewport dimensions in pixels.
     * @param camera       Active camera.
     * @param outOrigin    Output ray origin in world space.
     * @param outDir       Output normalised ray direction.
     */
    static void screenToWorldRay(const glm::vec2& mousePixel,
                                 const glm::vec2& viewportSize,
                                 const Camera& camera,
                                 glm::vec3& outOrigin,
                                 glm::vec3& outDir);

    /**
     * @brief Cast a ray against the terrain height field.
     * @param origin  Ray origin in world space.
     * @param dir     Normalised ray direction.
     * @param maxDist Maximum ray travel distance.
     * @param outHit  Output hit position in world space.
     * @return True if the ray intersected the terrain surface.
     */
    bool raycast(const glm::vec3& origin, const glm::vec3& dir,
                 float maxDist, glm::vec3& outHit) const;

    /// Returns the total terrain width in world units (X axis).
    float totalSizeX() const { return gridParams_.chunksX * gridParams_.chunkSizeM; }
    /// Returns the total terrain depth in world units (Z axis).
    float totalSizeZ() const { return gridParams_.chunksZ * gridParams_.chunkSizeM; }

    /**
     * @brief Sample terrain height at a world-space XZ position.
     * @param x World X coordinate.
     * @param z World Z coordinate.
     * @return Bilinearly interpolated height value.
     */
    float getHeightAt(float x, float z) const { return sampleHeight(x, z); }

    /**
     * @brief Sample the river influence mask at a world-space XZ position.
     * @param x World X coordinate.
     * @param z World Z coordinate.
     * @return Mask value in [0, 1]; values above ~0.15 indicate the river channel.
     */
    float riverMaskAt(float x, float z) const;

    /**
     * @brief Register terrain chunks as ECS entities with render components.
     * @param world    ECS world.
     * @param material Terrain render material.
     */
    void registerEntityInWorld(World& world, Material* material);

    /**
     * @brief Register the water surface mesh as an ECS entity.
     * @param world     ECS world.
     * @param waterMat  Water render material.
     * @param textures  Additional water textures (e.g. normal maps, foam).
     */
    void registerWaterInWorld(World& world, Material* waterMat,
                              const std::vector<Texture*>& textures);

  private:
    /// CPU-side data for a single chunk, produced during parallel generation.
    struct ChunkCPUData {
      std::array<std::vector<Vertex>, 4>       lodVertices; ///< Vertex data per LOD level.
      std::array<std::vector<unsigned int>, 4> lodIndices;  ///< Index data per LOD level.
      glm::vec3 worldOrigin;
      glm::vec3 aabbMin;
      glm::vec3 aabbMax;
      u32 cx = 0;
      u32 cz = 0;
    };

    float     sampleHeight(float x, float z)    const;
    float     sampleHeightRaw(float x, float z) const;
    void      buildHeightField();
    glm::vec3 calcNormal(float x, float z, float step) const;

    void buildAllChunks(GraphicsDevice& gd, JobSystem* js);
    void rebuildHeightFieldAndWater(GraphicsDevice& gd);

    std::unique_ptr<Mesh> buildLODMesh(GraphicsDevice& gd, u32 cx, u32 cz, u32 subs) const;
    TerrainChunk          buildChunk(GraphicsDevice& gd, u32 cx, u32 cz)               const;
    ChunkCPUData          buildChunkCPU(u32 cx, u32 cz)                                const;
    TerrainChunk          uploadChunkToGPU(GraphicsDevice& gd, const ChunkCPUData& data) const;

    std::vector<TerrainChunk>             chunks_;
    TerrainGridParams                     gridParams_;
    TerrainNoiseParams                    noiseParams_;
    TerrainSplatParams                    splatParams_;
    std::vector<std::unique_ptr<Texture>> splatTextures_;

    std::vector<unsigned char> heightmapData_;
    int hmWidth_  = 0;
    int hmHeight_ = 0;

    std::vector<float> heightField_;
    int   hfWidth_    = 0;
    int   hfHeight_   = 0;
    float hfCellSize_ = 0.f;

    std::unique_ptr<Mesh> waterMesh_;
    TerrainAABB           waterAABB_;

    River               river_;
    std::vector<Entity> entities_;
    Entity              waterEntity_;
    Material*           waterMaterial_;
    std::vector<Texture*> waterTextures_;
  };

  /**
   * @brief ECS system function that applies the terrain sculpting brush each frame.
   * @param ctx ECS context providing access to terrain, camera, and input state.
   */
  void terrainBrushSystem(Context& ctx);

} // namespace mam
