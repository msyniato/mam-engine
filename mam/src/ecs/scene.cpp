#include "ecs/scene.hpp"

namespace mam {

  SceneRegistry::SceneRegistry() :
    currentSceneID_(std::nullopt) {
    for (ID id = 0; id < kMaxScenes; id++) {
      idQueue_.push(id);
    }
  }

  Scene* SceneRegistry::createScene() {
    if (scenes_.size() >= kMaxScenes) {
      return nullptr;
    }

    ID newID = idQueue_.front();
    idQueue_.pop();

    auto scene = std::make_unique<Scene>();
    scene->id = newID;
    scene->context = nullptr;
    scene->onLoad = nullptr;
    scene->onUnload = nullptr;

    size_t index = scenes_.size();
    scenes_.push_back(std::move(scene));
    idToIndex_[newID] = index;

    currentSceneID_ = scenes_.back()->id;

    return scenes_.back().get();
  }

  void SceneRegistry::destroyScene(ID id) {
    auto item = idToIndex_.find(id);
    if (item == idToIndex_.end()) return;

    size_t index = item->second;

    if (currentSceneID_ == id) {
      if (scenes_.size() > 1)
        currentSceneID_ = scenes_.back()->id;
      else
        currentSceneID_.reset();
    }

    size_t lastIndex = scenes_.size() - 1;
    if (index != lastIndex) {
      std::swap(scenes_[index], scenes_[lastIndex]);
      idToIndex_[scenes_[index]->id] = index;
    }

    idToIndex_.erase(id);
    scenes_.pop_back();

    idQueue_.push(id);
  }

  Scene* SceneRegistry::getCurrentScene() {
    if (!currentSceneID_)
      return nullptr;

    auto it = idToIndex_.find(*currentSceneID_);
    return it != idToIndex_.end() ? scenes_[it->second].get() : nullptr;
  }

  void SceneRegistry::setCurrentScene(ID id) {
    auto it = idToIndex_.find(id);
    if (it != idToIndex_.end()) {
      currentSceneID_ = id;
    }
  }

  Scene* SceneRegistry::getScene(ID id) {
    auto item = idToIndex_.find(id);
    if (item == idToIndex_.end()) {
      return nullptr;
    }

    size_t index = item->second;
    return scenes_[index].get();
  }

}
