#pragma once

#include "common/common.hpp"
#include "ecs/component_array.hpp"
#include "ecs/entity.hpp"

namespace mam {

  /**
   * @brief Contiguous storage for all entities that share the same component signature.
   *
   * Each archetype owns one ComponentArray per component type in its signature.
   * Entities are stored in a parallel array so that component data for entity[i]
   * always resides at row i in every component array.
   */
  class Archetype {
  public:
    /**
     * @brief Construct an archetype for the given signature.
     * @param signature Bitmask of component types owned by this archetype.
     * @param cm        Component registry used to create component arrays.
     */
    Archetype(const Signature& signature, ComponentRegistry& cm);
    ~Archetype() = default;

    /// Returns the component signature bitmask for this archetype.
    const Signature& signature() const { return signature_; }

    /**
     * @brief Append an entity to this archetype and return its row index.
     * @param e Entity to add.
     * @return Row index assigned to the entity.
     */
    std::size_t addEntity(Entity e);

    /// Returns the number of entities stored in this archetype.
    std::size_t getEntitiesNum() const { return entities_.size(); }

    /**
     * @brief Remove the entity at the given row using swap-and-pop.
     * @param row Row of the entity to remove.
     * @return The entity that was moved into @p row (previously the last entity).
     */
    Entity removeEntity(std::size_t row);

    /**
     * @brief Return a typed component array for the given type ID.
     * @tparam T Component type.
     * @param type Numeric type ID.
     * @return Typed pointer to the component array, or nullptr if absent.
     */
    template <typename T>
    inline ComponentArray<T>* getArray(Type type) {
      return static_cast<ComponentArray<T>*>(getArray(type));
    }

    /**
     * @brief Return the raw (type-erased) component array for the given type ID.
     * @param type Numeric type ID.
     * @return Pointer to the IComponentArray, or nullptr if not present.
     */
    IComponentArray* getArray(Type type);

    /// Returns all entities stored in this archetype in row order.
    const std::vector<Entity>& entities() const { return entities_; }

  private:
    std::unordered_map<Type, std::unique_ptr<IComponentArray>> componentArrays_; ///< Per-type component storage.
    std::vector<Entity>                                        entities_;        ///< Parallel entity list.
    Signature                                                  signature_;       ///< Component bitmask.
  };

  /// Convenience alias for the list of all archetypes in a world.
  using ArchetypeList = std::vector<std::unique_ptr<Archetype>>;

  /**
   * @brief Manages entity-to-archetype mappings and component add/remove migrations.
   *
   * When a component is added to or removed from an entity, the registry
   * migrates the entity (and all its components) from its current archetype
   * to a new archetype whose signature matches the updated set.
   */
  class ArchetypeRegistry {
  public:
    /**
     * @brief Construct the registry with a reference to the component registry.
     * @param cm Component registry used for type look-ups and array creation.
     */
    ArchetypeRegistry(ComponentRegistry& cm);
    ~ArchetypeRegistry() = default;

    /**
     * @brief Add a component to an entity, migrating it to a new archetype.
     * @param e    Target entity.
     * @param type Numeric type ID of the component to add.
     * @param data Pointer to the component data to copy.
     */
    void addComponent(Entity e, Type type, const void* data);

    /**
     * @brief Remove a component from an entity, migrating it to a new archetype.
     * @param e    Target entity.
     * @param type Numeric type ID of the component to remove.
     */
    void removeComponent(Entity e, Type type);

    /**
     * @brief Return the archetype currently owning the given entity.
     * @param e Target entity.
     * @return Pointer to the entity's archetype.
     */
    Archetype* getEntityArchetype(Entity e) const { return entityLocations_[e].archetype; }

    /**
     * @brief Register a newly created entity in the empty archetype.
     * @param e Newly created entity.
     */
    void onEntityCreated(Entity e);

    /**
     * @brief Remove an entity from its archetype when it is destroyed.
     * @param e Entity being destroyed.
     */
    void onEntityDestroyed(Entity e);

    /**
     * @brief Check whether an entity has a component of the given type.
     * @param e    Target entity.
     * @param type Numeric type ID.
     * @return True if the entity's signature includes the type.
     */
    bool hasComponent(Entity e, Type type) const;

    /**
     * @brief Retrieve a typed component for an entity.
     * @tparam T Component type.
     * @param e Target entity.
     * @return Pointer to the component, or nullptr if not present.
     */
    template <typename T>
    T* getComponent(Entity e) const;

    /**
     * @brief Retrieve an untyped component pointer for an entity.
     * @param e    Target entity.
     * @param type Numeric type ID.
     * @return Raw pointer to the component, or nullptr if not present.
     */
    void* getComponent(Entity e, Type type) const;

    /// Returns all archetypes in this registry.
    const ArchetypeList& archetypes() const { return archetypes_; }

  private:
    /**
     * @brief Find or create the archetype for the given signature.
     * @param signature Component bitmask.
     * @return Pointer to the matching archetype.
     */
    Archetype* getArchetype(const Signature& signature);

    /// Per-entity location within an archetype.
    struct EntityLocation {
      Archetype*  archetype = nullptr; ///< Archetype owning the entity.
      std::size_t row       = 0;       ///< Row index within the archetype.
    };

    std::array<EntityLocation, kMaxEntities> entityLocations_; ///< Dense entity → location map.
    ComponentRegistry&                       ComponentRegistry_; ///< Reference to the component registry.
    ArchetypeList                            archetypes_;        ///< All archetypes in this world.
    Archetype*                               emptyArchetype_;    ///< Archetype with an empty signature.
  };

  template <typename T>
  inline T* ArchetypeRegistry::getComponent(Entity e) const {
    auto type = ComponentRegistry_.getComponentType<T>();
    if (!type)
      return nullptr;
    return static_cast<T*>(getComponent(e, *type));
  }

} // namespace mam
