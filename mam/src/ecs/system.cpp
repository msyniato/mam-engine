#include "ecs/system.hpp"

namespace mam {
  SystemRegistry::SystemRegistry()
    : nextID_(0) {
    systems_.reserve(kMaxSystems);
  }

  System* SystemRegistry::createSystem(const std::string& name, const Signature& signature) {
    if (systems_.size() >= kMaxSystems)
      return nullptr;

    auto system = std::make_unique<System>();
    system->id = nextID_++;
    system->name = name;
    system->requiredComponents = signature;

    size_t index = systems_.size();
    idToIndex_[system->id] = index;

    systems_.push_back(std::move(system));
    return systems_.back().get();
  }

  System* SystemRegistry::getSystem(ID id) {
    auto it = idToIndex_.find(id);
    if (it == idToIndex_.end())
      return nullptr;

    return systems_[it->second].get();
  }

  void SystemRegistry::removeSystem(ID id) {
    auto it = idToIndex_.find(id);
    if (it == idToIndex_.end())
      return;

    size_t index = it->second;
    size_t last = systems_.size() - 1;

    if (index != last) {
      std::swap(systems_[index], systems_[last]);
      idToIndex_[systems_[index]->id] = index;
    }

    idToIndex_.erase(id);
    systems_.pop_back();
  }
} //namespace mam
