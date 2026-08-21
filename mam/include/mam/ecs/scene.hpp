#pragma once

#include "common/common.hpp"

namespace mam {

  class World;

  /**
   * @brief A logical grouping of world state with load/unload lifecycle callbacks.
   *
   * Each scene holds an optional user context pointer and two function pointers
   * invoked when the scene becomes active or is deactivated.
   */
  struct Scene {
    ID   id      = 0;       ///< Unique scene identifier.
    void* context = nullptr; ///< Optional user-supplied context passed to callbacks.

    /// Called when this scene is made current. Receives the owning world and the user context.
    void (*onLoad)(World&, void*)   = nullptr;

    /// Called when this scene is deactivated. Receives the owning world and the user context.
    void (*onUnload)(World&, void*) = nullptr;
  };

  /**
   * @brief Manages the collection of scenes within a world.
   *
   * Scenes are created with auto-generated IDs, stored internally, and one
   * scene at a time may be designated as the current (active) scene.
   */
  class SceneRegistry {
  public:
    SceneRegistry();
    ~SceneRegistry() = default;

    /**
     * @brief Allocate a new empty scene and return a pointer to it.
     * @return Pointer to the newly created scene.
     */
    Scene* createScene();

    /**
     * @brief Look up a scene by its ID.
     * @param id Scene identifier.
     * @return Pointer to the scene, or nullptr if not found.
     */
    Scene* getScene(ID id);

    /**
     * @brief Return the currently active scene.
     * @return Pointer to the current scene, or nullptr if none has been set.
     */
    Scene* getCurrentScene();

    /**
     * @brief Make the scene with the given ID the active scene.
     * @param id ID of the scene to activate.
     */
    void setCurrentScene(ID id);

    /**
     * @brief Destroy the scene with the given ID and recycle its slot.
     * @param id ID of the scene to remove.
     */
    void destroyScene(ID id);

  private:
    std::vector<std::unique_ptr<Scene>> scenes_;       ///< Owned scene objects.
    std::unordered_map<ID, size_t>      idToIndex_;    ///< Maps scene ID to index in scenes_.
    std::queue<ID>                      idQueue_;      ///< Recycled IDs available for new scenes.
    std::optional<ID>                   currentSceneID_; ///< ID of the currently active scene, if any.
  };

} // namespace mam
