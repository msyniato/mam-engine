#pragma once

#include "common/common.hpp"
#include "core/utils.hpp"

namespace mam {

  /**
   * @brief Type-erased interface for a contiguous component column.
   *
   * Each archetype owns one IComponentArray per component type in its
   * signature. The interface allows the archetype to manipulate columns
   * without knowing the concrete component type at compile time.
   */
  class IComponentArray {
  public:
    virtual ~IComponentArray() = default;

    /**
     * @brief Remove the element at the given row using swap-and-pop.
     * @param row Index of the element to remove.
     */
    virtual void removeAt(size_t row) = 0;

    /**
     * @brief Append a copy of the element at row @p row from @p src.
     * @param src Source array of the same concrete type.
     * @param row Row index in @p src to copy.
     */
    virtual void copyFrom(IComponentArray* src, std::size_t row) = 0;

    /**
     * @brief Append a new element from an untyped data pointer.
     * @param data Pointer to a value of the concrete component type.
     */
    virtual void pushBackFrom(const void* data) = 0;

    /**
     * @brief Return an untyped pointer to the element at the given row.
     * @param row Row index.
     * @return Raw pointer to the component value.
     */
    virtual void* getRaw(std::size_t row) = 0;

    /// Returns the number of elements stored in this array.
    virtual size_t size() const = 0;
  };

  /// Factory function type that creates a new IComponentArray of a specific component type.
  using ComponentArrayFactory = std::function<std::unique_ptr<IComponentArray>()>;

  /**
   * @brief Type-safe, contiguous storage for a single component type.
   *
   * Backed by std::vector<T>; supports swap-and-pop removal, cross-array
   * copying, and raw-pointer access required by the archetype system.
   *
   * @tparam T Component type stored in this array.
   */
  template <typename T>
  class ComponentArray : public IComponentArray {
  public:
    /**
     * @brief Append a component value.
     * @param value Component to push.
     */
    void pushBack(const T& value);

    void   removeAt(size_t row)                          override;
    void   copyFrom(IComponentArray* src, std::size_t row) override;
    void   pushBackFrom(const void* data)                override;
    size_t size() const override { return data_.size(); }
    void*  getRaw(std::size_t row) override { return &data_[row]; }

    /**
     * @brief Return a typed reference to the element at the given row.
     * @param row Row index.
     * @return Reference to the component.
     */
    T& get(std::size_t row) { return *static_cast<T*>(getRaw(row)); }

  protected:
    std::vector<T> data_; ///< Underlying contiguous storage.
  };

  template <typename T>
  void ComponentArray<T>::pushBack(const T& value) {
    data_.push_back(value);
  }

  template <typename T>
  void ComponentArray<T>::removeAt(size_t row) {
    data_[row] = std::move(data_.back());
    data_.pop_back();
  }

  template <typename T>
  void ComponentArray<T>::copyFrom(IComponentArray* src, std::size_t row) {
    auto* typedSrc = static_cast<ComponentArray<T>*>(src);
    data_.push_back(typedSrc->data_[row]);
  }

  template <typename T>
  void ComponentArray<T>::pushBackFrom(const void* data) {
    data_.push_back(*static_cast<const T*>(data));
  }

  /**
   * @brief Central registry that maps component types to numeric type IDs and factory functions.
   *
   * Components must be registered once before they can be added to entities.
   * Registration assigns a stable Type ID, stores the component name, and
   * registers a factory for creating empty ComponentArray instances.
   */
  class ComponentRegistry {
  public:
    ComponentRegistry() : nextType_(0) {}
    ~ComponentRegistry() = default;

    /**
     * @brief Register a component type.
     *
     * Computes the FNV-1a hash of the type name, assigns a monotonically
     * increasing Type ID, and stores a factory for ComponentArray<T>.
     * Safe to call multiple times for the same type (idempotent).
     *
     * @tparam T Component type to register.
     */
    template <typename T>
    void registerComponent();

    /**
     * @brief Return the FNV-1a hash for a component type, if registered.
     * @tparam T Component type.
     * @return Hash wrapped in std::optional, or nullopt if not registered.
     */
    template <typename T>
    std::optional<Hash> getComponentHash() const;

    /**
     * @brief Return the numeric Type ID for a component type, if registered.
     * @tparam T Component type.
     * @return Type wrapped in std::optional, or nullopt if not registered.
     */
    template <typename T>
    std::optional<Type> getComponentType() const;

    /**
     * @brief Return the string name for a component type, if registered.
     * @tparam T Component type.
     * @return string_view wrapped in std::optional, or nullopt if not registered.
     */
    template <typename T>
    std::optional<std::string_view> getComponentName() const;

    /**
     * @brief Create a new, empty component array for the given type ID.
     * @param type Type ID previously assigned by registerComponent.
     * @return Owning pointer to the new array.
     */
    std::unique_ptr<IComponentArray> createComponentArray(Type type) const;

  private:
    std::optional<Type>             getComponentType(Hash id) const;
    std::optional<std::string_view> getComponentName(Hash id) const;

    std::unordered_map<Type, ComponentArrayFactory> factories_;         ///< Type ID → factory.
    std::unordered_map<Hash, std::string>           componentRegistry_; ///< Hash → type name.
    std::unordered_map<Type, Hash>                  typeToHash_;        ///< Type ID → hash.
    std::unordered_map<Hash, Type>                  hashToType_;        ///< Hash → type ID.
    Type                                            nextType_;          ///< Next type ID to assign.
  };

  template <typename T>
  void ComponentRegistry::registerComponent() {
    auto name = TypeName<T>::get();
    Hash hash = fnv1a(name);

    auto it = componentRegistry_.find(hash);
    if (it != componentRegistry_.end()) {
      if (it->second != name) {
        assert(false && "Hash collision detected!");
      }
      return;
    }

    if (nextType_ >= kMaxComponentTypes) {
      return;
    }

    componentRegistry_.emplace(hash, name);
    hashToType_.emplace(hash, nextType_);
    typeToHash_.emplace(nextType_, hash);
    factories_[nextType_] = [] {
      return std::make_unique<ComponentArray<T>>();
    };
    nextType_++;
  }

  template <typename T>
  std::optional<Hash> ComponentRegistry::getComponentHash() const {
    auto name = TypeName<T>::get();
    Hash hash = fnv1a(name);
    return (componentRegistry_.find(hash) != componentRegistry_.end())
             ? std::make_optional(hash) : std::nullopt;
  }

  template <typename T>
  std::optional<Type> ComponentRegistry::getComponentType() const {
    auto hash = getComponentHash<T>();
    if (!hash)
      return std::nullopt;
    return getComponentType(*hash);
  }

  template <typename T>
  std::optional<std::string_view> ComponentRegistry::getComponentName() const {
    auto hash = getComponentHash<T>();
    if (!hash)
      return std::nullopt;
    return getComponentName(*hash);
  }

} // namespace mam
