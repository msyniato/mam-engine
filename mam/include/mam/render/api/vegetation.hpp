#pragma once

#include "common/common.hpp"
#include "ecs/entity.hpp"

namespace mam {

  class GraphicsDevice;
  class Mesh;
  class World;
  class Material;
  class VertexBuffer;
  class Terrain;

  /**
   * @brief Parameters for the L-System procedural tree generator.
   */
  struct LSystemParams
  {
    std::string axiom = "X"; ///< Starting axiom string.

    std::unordered_map<char, std::string> rules; ///< Production rules mapping each symbol to its replacement string.

    float angle         = 25.f;  ///< Rotation angle in degrees applied per directional symbol.
    int   iterations    = 4;     ///< Number of L-System expansion iterations.
    float segmentLength = 1.6f;  ///< World-space length of each forward (F) segment.
    float segmentRadius = 0.1f;  ///< Base radius of the branch cylinder.
    float lengthDecay   = 0.7f;  ///< Length multiplier applied when entering a branch sub-tree ('[').
    float radiusDecay   = 0.6f;  ///< Radius multiplier applied when entering a branch sub-tree ('[').
    int   cylinderSides = 5;     ///< Number of sides per branch cylinder cross-section.
  };

  /**
   * @brief Parameters controlling vegetation instance placement on the terrain.
   */
  struct VegetationParams
  {
    u32   count            = 1000;   ///< Number of placement attempts.
    float minScale         = 4.0f;   ///< Minimum instance scale.
    float maxScale         = 8.0f;   ///< Maximum instance scale.
    float minHeightToSpawn = 5.0f;   ///< Minimum terrain height required for placement.
    float maxHeightToSpawn = 240.0f; ///< Maximum terrain height allowed for placement.
    u32   seed             = 42;     ///< Random seed for reproducible instance placement.
  };

  /**
   * @brief Generates a procedural tree mesh using an L-System grammar.
   *
   * Expands an axiom string through production rules for a fixed number of
   * iterations, then interprets the resulting string as drawing commands
   * to build cylinder-based branch geometry and leaf quads.
   */
  class LSystemTree {
  public:
    /**
     * @brief Construct an L-System tree with the given generation parameters.
     * @param params Axiom, rules, angles, segment dimensions, and decay factors.
     */
    explicit LSystemTree(const LSystemParams& params);
    ~LSystemTree() = default;

    /**
     * @brief Expand the axiom string through all production rule iterations.
     * @return Fully expanded L-System string.
     */
    std::string generate() const;

    /**
     * @brief Interpret an expanded L-System string and populate mesh buffers.
     * @param lsString   Expanded L-System string to interpret.
     * @param outVerts   Vertex buffer to append geometry to.
     * @param outIndices Index buffer to append indices to.
     */
    void buildMesh(const std::string& lsString,
                   std::vector<Vertex>& outVerts,
                   std::vector<unsigned int>& outIndices) const;

  private:
    /// Internal turtle state used during mesh construction.
    struct LSystemState {
      glm::vec3 pos;
      glm::vec3 heading = glm::vec3(0.0f, 1.0f, 0.0f); ///< Current forward direction.
      glm::vec3 left    = glm::vec3(1.0f, 0.0f, 0.0f);  ///< Current left direction.
      glm::vec3 up      = glm::vec3(0.0f, 0.0f, -1.0f); ///< Current up direction.
      float     length;                                   ///< Current segment length.
      float     radius;                                   ///< Current cylinder radius.
      int       depth = 0;                                ///< Branch nesting depth.
    };

    /**
     * @brief Append a tapered cylinder segment between two positions.
     * @param from    Start position in world space.
     * @param to      End position in world space.
     * @param r0      Radius at the start end.
     * @param r1      Radius at the tip end.
     * @param verts   Vertex buffer to append to.
     * @param indices Index buffer to append to.
     */
    void addCylinder(const glm::vec3& from, const glm::vec3& to,
                     float r0, float r1,
                     std::vector<Vertex>& verts,
                     std::vector<unsigned int>& indices) const;

    /**
     * @brief Append a double-sided leaf quad at the current turtle position.
     * @param t       Current turtle state.
     * @param verts   Vertex buffer to append to.
     * @param indices Index buffer to append to.
     */
    void addLeaf(const LSystemState& t,
                 std::vector<Vertex>& verts,
                 std::vector<unsigned int>& indices) const;

    /**
     * @brief Rotate a vector around an axis by the given angle (Rodrigues formula).
     * @param vec     Vector to rotate.
     * @param axis    Unit-length rotation axis.
     * @param radians Rotation angle in radians.
     * @return Rotated vector.
     */
    static glm::vec3 rotate(const glm::vec3& vec, const glm::vec3& axis, float radians);

    LSystemParams params_; ///< Generation parameters supplied at construction.
  };

  /**
   * @brief Scatters instanced L-System trees across a terrain surface.
   *
   * Generates the tree geometry once from an L-System, then distributes
   * instance transforms over the terrain using random placement filtered
   * by height range and river-proximity mask.
   * Registers itself as an ECS entity with a RenderComponent and an InstanceComponent.
   */
  class VegetationInstancer {
  public:
    /**
     * @brief Construct and fully initialise the vegetation instancer.
     * @param gd        Graphics device used to create GPU resources.
     * @param terrain   Terrain to query for height and river data.
     * @param params    L-System generation parameters.
     * @param vegParams Instance placement parameters.
     */
    VegetationInstancer(GraphicsDevice& gd,
                        const Terrain& terrain,
                        const LSystemParams& params,
                        const VegetationParams& vegParams);
    ~VegetationInstancer() = default;

    /**
     * @brief Initialise or re-initialise all GPU resources and instance matrices.
     * @param gd        Graphics device.
     * @param terrain   Terrain to scatter instances on.
     * @param params    L-System parameters.
     * @param vegParams Placement parameters.
     */
    void init(GraphicsDevice& gd,
              const Terrain& terrain,
              const LSystemParams& params,
              const VegetationParams& vegParams);

    /**
     * @brief Destroy the ECS entity and reset instance data.
     * @param world ECS world that owns the entity.
     */
    void clear(World& world);

    /**
     * @brief Register an ECS entity with RenderComponent and InstanceComponent.
     * @param world    ECS world to register into.
     * @param material Material used for rendering.
     */
    void registerEntitiesInWorld(World& world, Material* material);

    /**
     * @brief Recalcula solo las posiciones de las instancias sobre el terreno modificado.
     *
     * Reutiliza la misma semilla y parámetros de la última llamada a init(),
     * por lo que los árboles mantienen sus posiciones XZ pero se ajustan al
     * nuevo heightfield sin reconstruir la geometría del árbol.
     * @param gd      Graphics device para re-subir el buffer de instancias.
     * @param terrain Terreno con el heightfield actualizado.
     */
    void repositionOnTerrain(GraphicsDevice& gd, const Terrain& terrain, World& world);

    /// Returns the number of active tree instances.
    u32 instanceCount() const { return instanceCount_; }

    VegetationInstancer& operator=(VegetationInstancer&& other);
    VegetationInstancer& operator=(const VegetationInstancer&) = delete;

  private:
    std::unique_ptr<Mesh>         treeMesh_;        ///< Shared tree mesh geometry.
    std::unique_ptr<VertexBuffer> instancerBuffer_; ///< Per-instance mat4 transforms on the GPU.
    u32                           instanceCount_ = 0;
    Entity                        entity_;
    VegetationParams              vegParams_;       ///< Placement params guardados para repositionOnTerrain.
  };

} // namespace mam
