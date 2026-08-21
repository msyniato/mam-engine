#include "ecs/world.hpp"
#include "ecs/engine_components.hpp"
#include "ecs/engine_systems.hpp"
#include "ecs/light_probe.hpp"
#include "ecs/probe_systems.hpp"
#include "render/api/probe_baker.hpp"

namespace mam {

  World::World(ID id) {
    id_ = id;

    componentRegistry_ = std::make_unique<ComponentRegistry>();
    archetypeRegistry_ = std::make_unique<ArchetypeRegistry>(*componentRegistry_);
    entityRegistry_ = std::make_unique<EntityRegistry>();
    systemRegistry_ = std::make_unique<SystemRegistry>();
    sceneRegistry_ = std::make_unique<SceneRegistry>();

  }

  Entity World::createEntity(const std::string& name) {

    auto currentScene = sceneRegistry_->getCurrentScene();
    if (!currentScene)
      return kInvalidEntity;

    Entity newEntity = entityRegistry_->createEntity(name);
    archetypeRegistry_->onEntityCreated(newEntity);

    addComponent<SceneTagComponent>(newEntity, SceneTagComponent{ .id = currentScene->id });

    addComponent<TransformComponent>(newEntity, TransformComponent{
      .position = {0.0f, 0.0f, 0.0f},
      .rotation = {0.0f, 0.0f, 0.0f},
      .scale = {1.0f, 1.0f, 1.0f}
      });

    return newEntity;
  }

  EntityHandle World::getHandle(Entity e) {
    return { e, *this };
  }

  const std::string& World::getEntityName(Entity e) const {
    return entityRegistry_->getName(e);
  }

  void World::setEntityName(Entity e, const std::string& name) {
    entityRegistry_->setName(e, name);
  }

  EntityMetadata* World::getEntityMetadata(Entity e) {
    return entityRegistry_->getMetadata(e);
  }

  void World::destroyEntity(Entity e) {
    entityRegistry_->destroyEntity(e);

    archetypeRegistry_->onEntityDestroyed(e);
  }

  Scene* World::createScene() {
    return sceneRegistry_->createScene();
  }

  Scene* World::getScene(ID id) {
    return sceneRegistry_->getScene(id);
  }

  void World::destroyScene(ID id) {
    sceneRegistry_->destroyScene(id);

  }

  System* World::registerSystem(const std::string& name, const Signature& signature) {
    return systemRegistry_->createSystem(name, signature);
  }

  void World::updateSystems(Context& context) {
    auto& systems = systemRegistry_->systems();

    for (auto& system : systems) {
      if (system->name == "lights_system")
        system->onUpdate(context);
    }

    for (auto& system : systems) {
      if (system->name != "render_system" &&
        system->name != "lights_system" &&
        system->name != "probe_update_system" &&
        system->name != "probe_collect_system") {
        system->onUpdate(context);
      }
    }

    for (auto& system : systems) {
      if (system->name == "probe_update_system")
        system->onUpdate(context);
    }

    for (auto& system : systems) {
      if (system->name == "probe_collect_system")
        system->onUpdate(context);
    }

    for (auto& system : systems) {
      if (system->name == "render_system")
        system->onUpdate(context);
    }
  }

  Scene* World::getCurrentScene() const {
    return sceneRegistry_->getCurrentScene();
  }

  WorldRegistry::WorldRegistry() :
    currentWorldID_(std::nullopt)
  {
    for (ID id = 0; id < kMaxWorlds; id++) {
      idQueue_.push(id);
    }
  }

  World* WorldRegistry::createWorld()
  {

    if (worlds_.size() >= kMaxWorlds) return nullptr;

    ID newID = idQueue_.front();
    idQueue_.pop();
    auto world = std::make_unique<World>(newID);
    size_t index = worlds_.size();
    idToIndex_[world->getID()] = index;

    world->registerComponent<TransformComponent>();
    world->registerComponent<RenderComponent>();
    world->registerComponent<CameraComponent>();
    world->registerComponent<SceneTagComponent>();
    world->registerComponent<LightComponent>();
    world->registerComponent<LODComponent>();
    world->registerComponent<InstanceComponent>();
    world->registerComponent<BatchComponent>();
    world->registerComponent<LightProbeComponent>();

    Signature lightsSignature;
    lightsSignature.set(world->getComponentType<LightComponent>());
    auto lightingSystem = world->registerSystem("lights_system", lightsSignature);
    lightingSystem->onUpdate = collectLightsSystem;

    Signature renderSignature;
    renderSignature.set(world->getComponentType<RenderComponent>());
    renderSignature.set(world->getComponentType<TransformComponent>());
    auto renderSystem = world->registerSystem("render_system", renderSignature);
    renderSystem->onUpdate = updateRenderSystem;

    Signature probeBakeSignature;
    probeBakeSignature.set(world->getComponentType<LightProbeComponent>());
    probeBakeSignature.set(world->getComponentType<TransformComponent>());
    auto probeUpdateSys = world->registerSystem("probe_update_system", probeBakeSignature);
    probeUpdateSys->onUpdate = updateProbesSystem;

    Signature probeCollectSignature;
    probeCollectSignature.set(world->getComponentType<LightProbeComponent>());
    probeCollectSignature.set(world->getComponentType<TransformComponent>());
    auto probeCollectSys = world->registerSystem("probe_collect_system", probeCollectSignature);
    probeCollectSys->onUpdate = collectProbesSystem;

    worlds_.push_back(std::move(world));
    if (!currentWorldID_) currentWorldID_ = newID;

    return worlds_.back().get();
  }

  World* WorldRegistry::getCurrentWorld()
  {
    if (!currentWorldID_) return nullptr;

    auto it = idToIndex_.find(*currentWorldID_);
    return it != idToIndex_.end() ? worlds_[it->second].get() : nullptr;
  }

  void WorldRegistry::setCurrentWorld(ID id)
  {
    auto it = idToIndex_.find(id);
    if (it != idToIndex_.end()) {
      currentWorldID_ = id;
    }
  }

  World* WorldRegistry::getWorld(ID id)
  {
    auto item = idToIndex_.find(id);
    if (item == idToIndex_.end()) {
      return nullptr;
    }

    size_t index = item->second;
    return worlds_[index].get();
  }

  void WorldRegistry::destroyWorld(ID id)
  {
    auto item = idToIndex_.find(id);
    if (item == idToIndex_.end()) return;

    size_t index = item->second;

    if (currentWorldID_ == id){
      if (worlds_.size() > 1)
        currentWorldID_ = worlds_.back()->getID();
      else
        currentWorldID_.reset();
    }

    size_t lastIndex = worlds_.size() - 1;
    if (index != lastIndex){
      std::swap(worlds_[index], worlds_[lastIndex]);
      idToIndex_[worlds_[index]->getID()] = index;
    }

    idToIndex_.erase(id);
    worlds_.pop_back();

    idQueue_.push(id);
  }

} // namespace mam