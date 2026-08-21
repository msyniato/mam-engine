#include "ecs/archetype.hpp"
#include "ecs/component_array.hpp"

namespace mam {

  Archetype::Archetype(const Signature& signature, ComponentRegistry& cm)
    : signature_(signature) {
    for (Type t = 0; t < kMaxComponentTypes; t++) {
      if (signature_.test(t)) componentArrays_.emplace(t, cm.createComponentArray(t));
    }
  }

  std::size_t mam::Archetype::addEntity(Entity e) {
    entities_.push_back(e);
    return entities_.size() - 1;
  }

  Entity Archetype::removeEntity(std::size_t row) {
    std::size_t last = entities_.size() - 1;
    Entity moved = kInvalidEntity;

    if (row != last) {
      moved = entities_[last];
      entities_[row] = moved;
    }

    entities_.pop_back();

    for (auto& [type, array] : componentArrays_) {
      array->removeAt(row);
    }

    return moved;
  }

  IComponentArray* Archetype::getArray(Type type) {
    auto item = componentArrays_.find(type);
    if (item != componentArrays_.end()) return item->second.get();

    return nullptr;
  }
  
  ArchetypeRegistry::ArchetypeRegistry(ComponentRegistry& cm)
    : ComponentRegistry_(cm) {
    Signature empty{};
    auto emptyArch = std::make_unique<Archetype>(empty, ComponentRegistry_);
    emptyArchetype_ = emptyArch.get();
    archetypes_.push_back(std::move(emptyArch));
  }

  void ArchetypeRegistry::addComponent(Entity e, Type type, const void* data) {
    auto& loc = entityLocations_[e];
    Archetype* oldArchetype = loc.archetype;

    Signature oldSignature = oldArchetype->signature();
    if (oldSignature.test(type)) return;

    Signature newSignature = oldSignature;
    newSignature.set(type);

    Archetype* newArchetype = getArchetype(newSignature);
    auto newRow = newArchetype->addEntity(e);

    for (Type t = 0; t < kMaxComponentTypes; t++) {
      if (oldSignature.test(t)) {
        auto* srcArray = oldArchetype->getArray(t);
        auto* dstArray = newArchetype->getArray(t);
        dstArray->copyFrom(srcArray, loc.row);
      }
    }
    newArchetype->getArray(type)->pushBackFrom(data);

    Entity movedEntity = oldArchetype->removeEntity(loc.row);
    if (movedEntity != kInvalidEntity)
      entityLocations_[movedEntity].row = loc.row;

    loc = {newArchetype, newRow};
  }

  void ArchetypeRegistry::removeComponent(Entity e, Type type) {
    auto& loc = entityLocations_[e];
    Archetype* oldArchetype = loc.archetype;

    Signature oldSignature = oldArchetype->signature();

    if (!oldSignature.test(type))
      return;

    Signature newSignature = oldSignature;
    newSignature.reset(type);

    Archetype* newArchetype = getArchetype(newSignature);

    size_t newRow = newArchetype->addEntity(e);

    for (Type t = 0; t < kMaxComponentTypes; ++t) {
      if (!oldSignature.test(t) || t == type)
        continue;

      auto* src = oldArchetype->getArray(t);
      auto* dst = newArchetype->getArray(t);
      dst->copyFrom(src, loc.row);
    }

    Entity moved = oldArchetype->removeEntity(loc.row);
    if (moved != kInvalidEntity)
      entityLocations_[moved].row = loc.row;

    loc = {newArchetype, newRow};
  }

  Archetype* ArchetypeRegistry::getArchetype(const Signature& signature) {
    if (!signature.any())
      return emptyArchetype_;

    for (auto& archetype : archetypes_) {
      if (archetype->signature() == signature)
        return archetype.get();
    }

    auto newArchetype = std::make_unique<Archetype>(signature, ComponentRegistry_);
    Archetype* result = newArchetype.get();
    archetypes_.push_back(std::move(newArchetype));

    return result;
  }

  void ArchetypeRegistry::onEntityCreated(Entity e) {
    std::size_t row = emptyArchetype_->addEntity(e);
    entityLocations_[e] = {emptyArchetype_, row};
  }

  void ArchetypeRegistry::onEntityDestroyed(Entity e) {
    auto& loc = entityLocations_[e];
    Entity moved = loc.archetype->removeEntity(loc.row);
    if (moved != kInvalidEntity)
      entityLocations_[moved].row = loc.row;

    loc = {};
  }

  bool ArchetypeRegistry::hasComponent(Entity e, Type type) const {
    return entityLocations_[e].archetype->signature().test(type);
  }

  void* ArchetypeRegistry::getComponent(Entity e, Type type) const {
    auto& loc = entityLocations_[e];
    if (!loc.archetype)
      return nullptr;

    if (!loc.archetype->signature().test(type))
      return nullptr;

    if (auto* arr = loc.archetype->getArray(type))
      return arr->getRaw(loc.row);

    return nullptr;
  }

}
