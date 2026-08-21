#include "render/api/mesh.hpp"
#include "render/api/graphics_context.hpp"
#include "render/api/vertex_array.hpp"
#include "render/api/buffer.hpp"
#include "render/api/graphics_device.hpp"

#include "render/vulkan/vk_device.hpp"

#include "jobsys/dispatcher.hpp"

namespace mam
{

  MeshRegistry::MeshRegistry(GraphicsDevice& gd, JobSystem& jobSystem)
    : graphicsDevice_(gd), jobSystem_(jobSystem)
  {}

  ID MeshRegistry::allocateID()
  {
    return nextID_++;
  }

  ID MeshRegistry::submitOBJLoad(const std::string&  path,
                                  const std::string&  name,
                                  bool                doAsync,
                                  MeshReadyCallback   onReady)
  {
    // 1. Allocate entry and ID under lock
    ID id;
    Entry* entry = nullptr;
    {
      std::lock_guard<std::mutex> lk(mutex_);
      id = allocateID();
      auto e    = std::make_unique<Entry>();
      e->mesh   = std::make_unique<Mesh>(graphicsDevice_, name);
      e->onReady = std::move(onReady);
      e->state.store(LoadState::Pending, std::memory_order_relaxed);
      entry = e.get();
      entries_[id] = std::move(e);
    }

    auto job = [this, entry, path, id]()
    {
      entry->state.store(LoadState::Loading, std::memory_order_release);

      entry->mesh->LoadOBJ_CPU(path.c_str());

      if (entry->mesh->vertices().empty())
      {
        entry->errorMsg = "OBJ parse produced no vertices: " + path;
        entry->state.store(LoadState::Failed, std::memory_order_release);
        return;
      }

      entry->state.store(LoadState::Uploading, std::memory_order_release);

      Dispatcher::RunOnMain([entry, id]()
      {
        entry->mesh->UploadToGPU();
        entry->state.store(LoadState::Ready, std::memory_order_release);

        if (entry->onReady)
          entry->onReady(id, entry->mesh.get());
      });
    };

    if (doAsync)
    {
      entry->jobHandle = jobSystem_.submit_callable(std::move(job));
    }
    else
    {
      job();
      DrainMainQueue();
    }

    return id;
  }

  ID MeshRegistry::loadOBJAsync(const std::string& path,
                                 const std::string& name,
                                 MeshReadyCallback  onReady)
  {
    return submitOBJLoad(path, name, true, std::move(onReady));
  }

  ID MeshRegistry::loadOBJSync(const std::string& path, const std::string& name)
  {
    return submitOBJLoad(path, name, false, nullptr);
  }

  ID MeshRegistry::createFromData(const std::string& name,
                                   std::vector<Vertex>       vertices,
                                   std::vector<unsigned int> indices)
  {
    std::lock_guard<std::mutex> lk(mutex_);
    ID id = allocateID();

    auto e  = std::make_unique<Entry>();
    e->mesh = std::make_unique<Mesh>(
      graphicsDevice_, name,
      std::move(vertices),
      std::move(indices)
    );
    e->state.store(LoadState::Ready, std::memory_order_release);
    entries_[id] = std::move(e);
    return id;
  }

  ID MeshRegistry::createCube(const std::string& name, bool highQuality)
  {
    std::lock_guard<std::mutex> lk(mutex_);
    ID id = allocateID();

    auto e  = std::make_unique<Entry>();
    e->mesh = std::make_unique<Mesh>(graphicsDevice_, name);

    if (highQuality)
      e->mesh->createCube24v();
    else
      e->mesh->createCube8v();

    e->state.store(LoadState::Ready, std::memory_order_release);
    entries_[id] = std::move(e);
    return id;
  }

  ID MeshRegistry::createQuad(const std::string& name)
  {
    std::lock_guard<std::mutex> lk(mutex_);
    ID id = allocateID();

    auto e  = std::make_unique<Entry>();
    e->mesh = std::make_unique<Mesh>(graphicsDevice_, name);
    e->mesh->createQuad();
    e->state.store(LoadState::Ready, std::memory_order_release);
    entries_[id] = std::move(e);
    return id;
  }

  ID MeshRegistry::createSphere(const std::string& name,
                                  int num_heights, int num_revs)
  {
    std::lock_guard<std::mutex> lk(mutex_);
    ID id = allocateID();

    auto e  = std::make_unique<Entry>();
    e->mesh = std::make_unique<Mesh>(graphicsDevice_, name);
    e->mesh->createSphere(num_heights, num_revs);
    e->state.store(LoadState::Ready, std::memory_order_release);
    entries_[id] = std::move(e);
    return id;
  }

  void MeshRegistry::drainPendingUploads()
  {
    DrainMainQueue();
  }

  Mesh* MeshRegistry::getMesh(ID id)
  {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = entries_.find(id);
    if (it == entries_.end()) return nullptr;
    if (it->second->state.load(std::memory_order_acquire) != LoadState::Ready)
      return nullptr;
    return it->second->mesh.get();
  }

  const Mesh* MeshRegistry::getMesh(ID id) const
  {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = entries_.find(id);
    if (it == entries_.end()) return nullptr;
    if (it->second->state.load(std::memory_order_acquire) != LoadState::Ready)
      return nullptr;
    return it->second->mesh.get();
  }

  Mesh* MeshRegistry::getMeshByName(const std::string& name)
  {
    std::lock_guard<std::mutex> lk(mutex_);
    for (auto& [id, entry] : entries_)
    {
      if (entry->mesh && entry->mesh->getName() == name &&
          entry->state.load(std::memory_order_acquire) == LoadState::Ready)
        return entry->mesh.get();
    }
    return nullptr;
  }

  MeshRegistry::LoadState MeshRegistry::getLoadState(ID id) const
  {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = entries_.find(id);
    if (it == entries_.end()) return LoadState::Failed;
    return it->second->state.load(std::memory_order_acquire);
  }

  std::size_t MeshRegistry::countReady() const
  {
    std::lock_guard<std::mutex> lk(mutex_);
    std::size_t n = 0;
    for (auto& [id, e] : entries_)
      if (e->state.load(std::memory_order_relaxed) == LoadState::Ready) ++n;
    return n;
  }

  void MeshRegistry::destroy(ID id)
  {
    std::unique_ptr<Entry> toDestroy;
    {
      std::lock_guard<std::mutex> lk(mutex_);
      auto it = entries_.find(id);
      if (it == entries_.end()) return;
      toDestroy = std::move(it->second);
      entries_.erase(it);
    }

    if (toDestroy->jobHandle.valid())
      toDestroy->jobHandle.wait();

  }

  void MeshRegistry::destroyAll()
  {
    std::unordered_map<ID, std::unique_ptr<Entry>> snapshot;
    {
      std::lock_guard<std::mutex> lk(mutex_);
      snapshot = std::move(entries_);
      entries_.clear();
    }

    for (auto& [id, e] : snapshot)
      if (e->jobHandle.valid())
        e->jobHandle.wait();

  }

} // namespace mam
