#pragma once

#include "common/common.hpp"

namespace mam {

  class World;

  /**
   * @brief Lightweight entity identifier.
   *
   * Wraps a numeric ID and provides validity checks and comparison operators.
   * An entity is considered invalid when its ID equals kInvalidID.
   */
  class Entity {
  public:
    /// Constructs an invalid entity.
    Entity() : id_(kInvalidID) {}

    /// Constructs an entity with the given ID.
    constexpr Entity(ID id) : id_(id) {}

    /// Returns the underlying numeric ID.
    ID id() const { return id_; }

    /// Returns true if the entity ID is not kInvalidID.
    bool isValid() const { return id_ != kInvalidID; }

    /// Implicit conversion to the underlying ID type.
    operator ID() const { return id_; }

    bool operator==(const Entity& o) const { return id_ == o.id_; }
    bool operator!=(const Entity& o) const { return id_ != o.id_; }

  private:
    ID id_; ///< Underlying numeric entity identifier.
  };

  /// Sentinel value representing an entity that does not exist.
  constexpr Entity kInvalidEntity{ std::numeric_limits<u32>::max() };

  /**
   * @brief Non-component metadata attached to every entity.
   *
   * Stores the human-readable name and active flag used by editor and debug tools.
   */
  struct EntityMetadata {
    std::string name;    ///< Human-readable name of the entity.
    bool active = true;  ///< Whether the entity participates in system updates.
  };

  /**
   * @brief Registry that owns and manages the lifecycle of all entities.
   *
   * Allocates entity IDs from a free-list queue, stores per-entity metadata,
   * and provides name look-up and destruction.
   */
  class EntityRegistry {
  public:
    EntityRegistry();
    ~EntityRegistry() = default;

    /**
     * @brief Create a new entity with the given name.
     * @param name Human-readable name (defaults to "Entity").
     * @return The newly created entity.
     */
    Entity createEntity(const std::string& name = "Entity");

    /**
     * @brief Destroy an entity and recycle its ID.
     * @param e Entity to destroy.
     */
    void destroyEntity(Entity e);

    /**
     * @brief Return the mutable metadata for an entity.
     * @param e Target entity.
     * @return Pointer to its metadata, or nullptr if not found.
     */
    EntityMetadata* getMetadata(Entity e);

    /**
     * @brief Return the immutable metadata for an entity.
     * @param e Target entity.
     * @return Const pointer to its metadata, or nullptr if not found.
     */
    const EntityMetadata* getMetadata(Entity e) const;

    /**
     * @brief Set the human-readable name of an entity.
     * @param e    Target entity.
     * @param name New name.
     */
    void setName(Entity e, const std::string& name);

    /**
     * @brief Return the name of an entity.
     * @param e Target entity.
     * @return Reference to the entity's name string.
     */
    const std::string& getName(Entity e) const;

  private:
    std::unordered_map<u32, EntityMetadata> metadata_;  ///< Per-entity metadata keyed by entity ID.
    std::queue<Entity>                      entityQueue_; ///< Recycled entity IDs available for reuse.
    u32                                     entityCount_; ///< Total number of live entities.
  };

  /**
   * @brief Convenience wrapper that couples an Entity with a World reference.
   *
   * Provides a fluent API for adding, removing, and querying components
   * and metadata without holding a direct reference to the registry internals.
   */
  class EntityHandle {
  public:
    /**
     * @brief Construct a handle for the given entity in the given world.
     * @param entity Entity to wrap.
     * @param world  World that owns the entity.
     */
    EntityHandle(Entity entity, World& world)
      : entity_(entity), world_(world) {}

    /// Returns the wrapped entity.
    Entity entity() const { return entity_; }

    /// Returns true if the underlying entity is valid.
    bool isValid() const { return entity_.isValid(); }

    /// Returns the name of the entity.
    const std::string& name() const;

    /**
     * @brief Set the human-readable name of the entity.
     * @param name New name.
     */
    void setName(const std::string& name);

    /// Returns whether the entity is active.
    bool isActive() const;

    /**
     * @brief Enable or disable the entity.
     * @param active True to activate, false to deactivate.
     */
    void setActive(bool active);

    /**
     * @brief Retrieve a component attached to this entity.
     * @tparam T Component type.
     * @return Pointer to the component, or nullptr if not attached.
     */
    template <typename T>
    T* getComponent();

    /**
     * @brief Attach a component to this entity.
     * @tparam T    Component type.
     * @param comp  Component value to copy into the archetype.
     */
    template <typename T>
    void addComponent(const T& comp);

    /**
     * @brief Remove a component from this entity.
     * @tparam T Component type to remove.
     */
    template <typename T>
    void removeComponent();

    /**
     * @brief Check whether this entity has a particular component type registered.
     * @tparam T Component type.
     * @return True if the component type is registered in the world.
     */
    template <typename T>
    bool hasComponent() const;

  private:
    Entity entity_; ///< The wrapped entity.
    World& world_;  ///< World that owns the entity.
  };

} // namespace mam
