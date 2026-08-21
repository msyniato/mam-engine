#pragma once

#include "common/common.hpp"
#include "sol/sol.hpp"
#include "glm/glm.hpp"

namespace mam {

  class Mesh;
  class Camera;
  class GraphicsDevice;
  class Material;
  class Texture;
  class VertexBuffer;

  /**
   * @brief Spatial transform component.
   *
   * Stores the position, rotation, and scale of an entity
   * in world or local space.
   */
  struct TransformComponent {
    glm::vec3 position; ///< World position of the entity.
    glm::vec3 rotation; ///< Rotation of the entity (Euler angles in radians).
    glm::vec3 scale;    ///< Uniform or per-axis scale of the entity.
  };

  /**
   * @brief Rendering component.
   *
   * Associates an entity with the mesh, material, and textures used
   * during the forward rendering pass, along with culling bounds.
   */
  struct RenderComponent {
    Mesh*                 mesh;                  ///< Mesh resource used to render the entity.
    Material*             material;              ///< Material applied to the mesh.
    std::vector<Texture*> textures;              ///< Ordered texture bindings for the material.
    glm::vec3             aabbMin;               ///< Minimum corner of the axis-aligned bounding box.
    glm::vec3             aabbMax;               ///< Maximum corner of the axis-aligned bounding box.
    glm::vec3             boundingSphereCenter;  ///< Center of the bounding sphere in local space.
    float                 boundingSphereRadius;  ///< Radius of the bounding sphere.
    bool                  useSplatting = false;  ///< True for terrain chunks that use splat-blended textures.
  };

  /**
   * @brief Tags an entity as belonging to a particular scene.
   */
  struct SceneTagComponent {
    ID id; ///< ID of the scene this entity belongs to.
  };

  /**
   * @brief Camera marker component.
   *
   * Identifies an entity as a camera. Camera-related
   * systems use this component to determine the active
   * view or projection.
   */
  struct CameraComponent {
  };

  /**
   * @brief Lighting component used by rendering systems.
   *
   * Stores lighting parameters for an entity. The structure
   * is padded to maintain 16-byte alignment, which is
   * required when sending data to GPU uniform buffers.
   */
  struct LightComponent {
    float     constant_att;   ///< Constant attenuation factor.
    glm::vec3 ambient;        ///< Ambient light color.
    float     linear_att;     ///< Linear attenuation factor.
    glm::vec3 diffuse;        ///< Diffuse light color.
    float     quadratic_att;  ///< Quadratic attenuation factor.
    glm::vec3 specular;       ///< Specular light color.
    float     cut_off;        ///< Inner spotlight cutoff angle (cosine).
    glm::vec3 direction;      ///< Light direction (used by directional and spot lights).
    float     outer_cut_off;  ///< Outer spotlight cutoff angle (cosine).
    LightType type;           ///< Light type (point, directional, spot).
  };

  /**
   * @brief Runtime Lua script execution context.
   *
   * Stores the Lua environment and lifecycle functions
   * associated with a script attached to an entity.
   */
  struct ScriptContext {
    sol::environment env;         ///< Lua environment for the script.
    sol::function    init;        ///< Script initialisation function.
    sol::function    update;      ///< Script update function (called each frame).
    sol::function    destroy;     ///< Script cleanup function.
    bool             initialized = false; ///< Whether the script has been initialised.

    /**
     * @brief Construct a script environment that inherits from the Lua global table.
     * @param lua Lua state used to create the environment.
     */
    ScriptContext(sol::state& lua)
      : env(lua, sol::create, lua.globals()) {}
  };

  /**
   * @brief Component that attaches a Lua script to an entity.
   *
   * Stores the script file path and its runtime execution context.
   */
  struct ScriptComponent {
    std::string                    file; ///< Path to the Lua script file.
    std::shared_ptr<ScriptContext> ctx;  ///< Runtime script execution context.
  };

  /**
   * @brief Level-of-detail component holding multiple mesh resolutions.
   *
   * Selects the appropriate mesh based on the distance from the camera
   * at render time. Up to MAX_LEVELS LOD levels are supported.
   */
  struct LODComponent {
    static constexpr int MAX_LEVELS = 4;                       ///< Maximum supported LOD levels.
    std::array<Mesh*, MAX_LEVELS>  meshes;                     ///< Meshes from highest to lowest detail.
    std::array<float, MAX_LEVELS>  distances;                  ///< Camera distances at which each LOD activates.
    glm::vec3                      center;                     ///< Local-space center used for distance calculation.
    int                            levels = MAX_LEVELS;        ///< Number of active LOD levels.
  };

  /**
   * @brief Marks an entity as an instanced draw call.
   *
   * The instance buffer contains per-instance transform data uploaded to the GPU.
   */
  struct InstanceComponent {
    VertexBuffer* instanceBuffer; ///< GPU buffer holding per-instance attributes.
    u32           instanceCount;  ///< Number of instances to draw.
  };

  /**
   * @brief Tags an entity as part of a static geometry batch.
   *
   * All entities sharing the same batchGroupID are merged into a single
   * draw call by the StaticBatcher.
   */
  struct BatchComponent {
    u32 batchGroupID; ///< Identifier of the batch group this entity belongs to.
  };

} // namespace mam
