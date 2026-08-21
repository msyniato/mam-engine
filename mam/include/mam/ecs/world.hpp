#pragma once

#include "common/common.hpp"
#include "ecs/component_array.hpp"
#include "ecs/archetype.hpp"
#include "ecs/entity.hpp"
#include "ecs/system.hpp"
#include "ecs/scene.hpp"

namespace mam {

  class Context;

  /**
   * @brief Top-level ECS container that owns entities, components, systems, and scenes.
   *
   * A World aggregates the five sub-registries (component, archetype, entity, system,
   * scene) and exposes a unified API for creating/destroying entities, attaching
   * components, registering systems, and managing scenes.
   */
  class World {
  public:
    /**
     * @brief Construct a world with the given unique ID.
     * @param id Identifier assigned by the WorldRegistry.
     */
    World(ID id);
    ~World() = default;

    /**
     * @brief Create a new entity and return it.
     * @param name Human-readable name (defaults to "Entity").
     * @return Newly created entity.
     */
    Entity createEntity(const std::string& name = "Entity");

    /**
     * @brief Return a handle that wraps the entity and a reference to this world.
     * @param e Target entity.
     * @return EntityHandle for the given entity.
     */
    EntityHandle getHandle(Entity e);

    /**
     * @brief Return the name of an entity.
     * @param e Target entity.
     */
    const std::string& getEntityName(Entity e) const;

    /**
     * @brief Set the human-readable name of an entity.
     * @param e    Target entity.
     * @param name New name.
     */
    void setEntityName(Entity e, const std::string& name);

    /**
     * @brief Return the mutable metadata for an entity.
     * @param e Target entity.
     * @return Pointer to the metadata, or nullptr if not found.
     */
    EntityMetadata* getEntityMetadata(Entity e);

    /**
     * @brief Destroy an entity and release all its components.
     * @param e Entity to destroy.
     */
    void destroyEntity(Entity e);

    /**
     * @brief Create a new scene in this world.
     * @return Pointer to the newly created scene.
     */
    Scene* createScene();

    /**
     * @brief Look up a scene by its ID.
     * @param id Scene identifier.
     * @return Pointer to the scene, or nullptr if not found.
     */
    Scene* getScene(ID id);

    /// Returns the currently active scene, or nullptr if none has been set.
    Scene* getCurrentScene() const;

    /**
     * @brief Destroy the scene with the given ID.
     * @param id ID of the scene to remove.
     */
    void destroyScene(ID id);

    /**
     * @brief Register a new system with the required component signature.
     * @param name      Human-readable system name.
     * @param signature Required component bitmask.
     * @return Pointer to the newly registered system.
     */
    System* registerSystem(const std::string& name, const Signature& signature);

    /**
     * @brief Run all registered systems' onUpdate callbacks.
     * @param context Application context forwarded to each callback.
     */
    void updateSystems(Context& context);

    /// Returns this world's unique identifier.
    ID getID() { return id_; }

    /**
     * @brief Register a component type so it can be added to entities.
     * @tparam T Component type to register.
     */
    template <typename T>
    void registerComponent() const;

    /**
     * @brief Add a component to an entity, migrating it to a new archetype.
     * @tparam T    Component type.
     * @param e     Target entity.
     * @param comp  Component value to copy.
     */
    template <typename T>
    void addComponent(Entity e, const T& comp);

    /**
     * @brief Remove a component from an entity, migrating it to a new archetype.
     * @tparam T Component type to remove.
     * @param e  Target entity.
     */
    template <typename T>
    void removeComponent(Entity e);

    /**
     * @brief Retrieve a component attached to an entity.
     * @tparam T Component type.
     * @param e  Target entity.
     * @return Pointer to the component, or nullptr if not present.
     */
    template <typename T>
    T* getComponent(Entity e);

    /**
     * @brief Return the numeric Type ID for a registered component type.
     * @tparam T Component type.
     * @return Numeric Type ID (asserts if the type is not registered).
     */
    template <typename T>
    Type getComponentType();

    /// Returns all archetypes currently alive in this world.
    const ArchetypeList& getArchetypes() const;

    double lastDeltaTime = 0.0; ///< Delta time (seconds) of the most recent update tick.

  private:
    std::unique_ptr<ComponentRegistry> componentRegistry_; ///< Owns component type registration.
    std::unique_ptr<ArchetypeRegistry> archetypeRegistry_; ///< Owns entity-to-archetype mappings.
    std::unique_ptr<EntityRegistry>    entityRegistry_;    ///< Owns entity metadata and IDs.
    std::unique_ptr<SystemRegistry>    systemRegistry_;    ///< Owns registered systems.
    std::unique_ptr<SceneRegistry>     sceneRegistry_;     ///< Owns scenes.
    ID                                 id_;                ///< Unique world identifier.
  };

  template <typename T>
  void World::addComponent(Entity e, const T& comp) {
    auto type = componentRegistry_->getComponentType<T>();
    if (!type)
      return;
    archetypeRegistry_->addComponent(e, *type, &comp);
  }

  template <typename T>
  void World::removeComponent(Entity e) {
    auto type = componentRegistry_->getComponentType<T>();
    if (!type)
      return;
    archetypeRegistry_->removeComponent(e, *type);
  }

  template <typename T>
  T* World::getComponent(Entity e) {
    auto type = componentRegistry_->getComponentType<T>();
    if (!type)
      return nullptr;
    return static_cast<T*>(archetypeRegistry_->getComponent(e, *type));
  }

  template <typename T>
  Type World::getComponentType() {
    auto componentType = componentRegistry_->getComponentType<T>();
    assert(componentType && "Invalid component type!");
    return *componentType;
  }

  template <typename T>
  void World::registerComponent() const {
    componentRegistry_->registerComponent<T>();
  }

  inline const ArchetypeList& World::getArchetypes() const {
    return archetypeRegistry_->archetypes();
  }

  template <typename T>
  T* EntityHandle::getComponent() {
    return world_.getComponent<T>(entity_);
  }

  template <typename T>
  void EntityHandle::addComponent(const T& comp) {
    world_.addComponent<T>(entity_, comp);
  }

  template <typename T>
  void EntityHandle::removeComponent() {
    world_.removeComponent<T>(entity_);
  }

  template <typename T>
  bool EntityHandle::hasComponent() const {
    if (world_.getComponentType<T>()) return true;
    return false;
  }

  /**
   * @brief Registry that manages multiple worlds and tracks the active one.
   *
   * Worlds are created with auto-generated IDs. One world at a time may be
   * designated as the current world and accessed without an explicit ID.
   */
  class WorldRegistry {
  public:
    WorldRegistry();
    ~WorldRegistry() = default;

    /**
     * @brief Allocate and register a new world.
     * @return Pointer to the newly created world.
     */
    World* createWorld();

    /// Returns the currently active world, or nullptr if none has been set.
    World* getCurrentWorld();

    /**
     * @brief Make the world with the given ID the active world.
     * @param id ID of the world to activate.
     */
    void setCurrentWorld(ID id);

    /**
     * @brief Look up a world by its ID.
     * @param id World identifier.
     * @return Pointer to the world, or nullptr if not found.
     */
    World* getWorld(ID id);

    /**
     * @brief Destroy a world and release all its resources.
     * @param id ID of the world to destroy.
     */
    void destroyWorld(ID id);

  private:
    std::vector<std::unique_ptr<World>> worlds_;        ///< Owned world objects.
    std::unordered_map<ID, size_t>      idToIndex_;     ///< Maps world ID to index in worlds_.
    std::optional<ID>                   currentWorldID_; ///< ID of the currently active world, if any.
    std::queue<ID>                      idQueue_;        ///< Recycled IDs available for new worlds.
  };

} // namespace mam
