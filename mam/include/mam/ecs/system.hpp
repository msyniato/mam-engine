#pragma once

#include "common/common.hpp"

namespace mam {

  class World;

  /**
   * @brief Type-erased service locator passed to every ECS system callback.
   *
   * Services (graphics device, renderer, etc.) are registered by pointer under
   * their type hash and retrieved with a static downcast via Get<T>().
   */
  class Context {
  public:
    /**
     * @brief Register a service of type T.
     * @tparam T Service type.
     * @param service Raw pointer to the service instance.
     */
    template<typename T>
    void Add(T* service) {
      context_[typeid(T).hash_code()] = service;
    }

    /**
     * @brief Retrieve a previously registered service of type T.
     * @tparam T Service type.
     * @return Pointer to the service, or nullptr if not registered.
     */
    template<typename T>
    T* Get() {
      auto it = context_.find(typeid(T).hash_code());
      if (it == context_.end())
        return nullptr;
      return static_cast<T*>(it->second);
    }

  private:
    std::unordered_map<size_t, void*> context_; ///< Type-hash → service pointer map.
  };

  /// Callback signature invoked once before a system's first update.
  using onUpdateCallback = void(*)(Context& context);

  /// Callback signature invoked each frame for a system update.
  using onBeginCallback  = void(*)(Context& context);

  /// Callback signature invoked once after a system is removed.
  using onEndCallback    = void(*)(Context& context);

  /**
   * @brief Data record describing a single ECS system.
   *
   * A system filters entities via a component signature bitmask and
   * executes up to three lifecycle callbacks (begin, update, end).
   */
  struct System {
    ID               id;                          ///< Unique system identifier.
    std::string      name;                        ///< Human-readable system name.
    onBeginCallback  onBegin  = nullptr;          ///< Called once before the first update.
    onUpdateCallback onUpdate = nullptr;          ///< Called every frame.
    onEndCallback    onEnd    = nullptr;          ///< Called once when the system is removed.
    Signature        requiredComponents = 0;      ///< Bitmask of component types this system requires.
  };

  /**
   * @brief Manages the ordered collection of systems within a world.
   *
   * Systems are created, looked up by ID or name, and removed through
   * this registry. They are updated in insertion order each frame.
   */
  class SystemRegistry {
  public:
    SystemRegistry();
    ~SystemRegistry() = default;

    /// Returns all registered systems in insertion order.
    const std::vector<std::unique_ptr<System>>& systems() { return systems_; }

    /**
     * @brief Create and register a new system.
     * @param name      Human-readable system name.
     * @param signature Required component bitmask.
     * @return Pointer to the newly created system.
     */
    System* createSystem(const std::string& name, const Signature& signature);

    /**
     * @brief Look up a system by its numeric ID.
     * @param id System identifier.
     * @return Pointer to the system, or nullptr if not found.
     */
    System* getSystem(ID id);

    /**
     * @brief Look up a system by its name.
     * @param name System name.
     * @return Pointer to the system, or nullptr if not found.
     */
    System* getSystem(const std::string& name);

    /**
     * @brief Remove a system from the registry.
     * @param id ID of the system to remove.
     */
    void removeSystem(ID id);

  private:
    std::vector<std::unique_ptr<System>> systems_;    ///< Owned system objects in insertion order.
    std::unordered_map<ID, size_t>       idToIndex_;  ///< Maps system ID to index in systems_.
    ID                                   nextID_;      ///< Next ID to assign to a new system.
  };

} // namespace mam
