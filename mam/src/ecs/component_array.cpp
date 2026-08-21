#include "ecs/component_array.hpp"

namespace mam {
  
  std::optional<Type> ComponentRegistry::getComponentType(Hash id) const {
    auto item = hashToType_.find(id);
    return (item != hashToType_.end()) ? std::make_optional(item->second) : std::nullopt;
  }

  std::optional<std::string_view> ComponentRegistry::getComponentName(Hash id) const {
    auto item = componentRegistry_.find(id);
    return (item != componentRegistry_.end()) ? std::make_optional<std::string_view>(item->second) : std::nullopt;
  }

  std::unique_ptr<IComponentArray> ComponentRegistry::createComponentArray(Type type) const {
    auto item = factories_.find(type);
    if (item == factories_.end()) return nullptr;

    return item->second();
  }
  
} //namespace mam
