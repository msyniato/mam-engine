#include "ecs/entity.hpp"

#include "ecs/world.hpp"

namespace mam {


  EntityRegistry::EntityRegistry() {
    for (Entity e = Entity(0); e.id() < kMaxEntities; e = Entity(e.id() + 1))
      entityQueue_.push(e);
    entityCount_ = 0;
  }

  Entity EntityRegistry::createEntity(const std::string& name) {
    Entity newEntity = entityQueue_.front();
    entityQueue_.pop();
    entityCount_++;

    metadata_[newEntity.id()] = EntityMetadata{ name };
    return newEntity;
  }

  void EntityRegistry::destroyEntity(Entity e) {
    metadata_.erase(e.id());
    entityQueue_.push(e);
    entityCount_--;
  }

  EntityMetadata* EntityRegistry::getMetadata(Entity e) {
    auto it = metadata_.find(e.id());
    return it != metadata_.end() ? &it->second : nullptr;
  }

  const EntityMetadata* EntityRegistry::getMetadata(Entity e) const {
    auto it = metadata_.find(e.id());
    return it != metadata_.end() ? &it->second : nullptr;
  }

  void EntityRegistry::setName(Entity e, const std::string& name) {
    if (auto* m = getMetadata(e)) m->name = name;
  }

  const std::string& EntityRegistry::getName(Entity e) const {
    static const std::string empty;
    auto* m = getMetadata(e);
    return m ? m->name : empty;
  }
  
  const std::string& EntityHandle::name() const {
    return world_.getEntityName(entity_);
  }
  void EntityHandle::setName(const std::string& n) {
    world_.setEntityName(entity_, n);
  }
  bool EntityHandle::isActive() const {
    auto* m = world_.getEntityMetadata(entity_);
    return m ? m->active : false;
  }
  void EntityHandle::setActive(bool active) {
    if (auto* m = world_.getEntityMetadata(entity_)) m->active = active;
  }

}
