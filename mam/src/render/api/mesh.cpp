
#include "render/api/mesh.hpp"
#include "render/api/graphics_context.hpp"
#include "render/api/vertex_array.hpp"
#include "render/api/buffer.hpp"
#include "render/api/graphics_device.hpp"

#include "render/vulkan/vk_device.hpp"

#include "jobsys/dispatcher.hpp"

namespace mam
{

  glm::vec3 normalPoint(const glm::vec3 &p1, const glm::vec3 &p2, const glm::vec3 &p3)
  {
    glm::vec3 tan_1 = p2 - p1;
    glm::vec3 tan_2 = p3 - p1;
    glm::vec3 norm = glm::cross(tan_2, tan_1);
    norm = glm::normalize(norm);
    return norm;
  }

  Mesh::Mesh(GraphicsDevice& gd, const std::string &name) :
    graphicsDevice_(gd)
  {
    name_ = name;
  }

  Mesh::Mesh(GraphicsDevice& gd, const std::string &name, std::vector<Vertex> vertices, std::vector<unsigned int> indices) :
    graphicsDevice_(gd)
  {
    vertices_ = vertices;
    indices_ = indices;
    name_ = name;
    initMesh();
  }

  Mesh::~Mesh()
  {
    if (vertexBuffer_) vertexBuffer_->release();
    if (indexBuffer_)  indexBuffer_->release();

  }

  std::string Mesh::getName() const {
    return name_;
  }

  void Mesh::LoadOBJ(const char* path)
{
    if (!path) return;

    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warnings, errors;

    if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warnings, &errors, path))
        return;

    vertices_.clear();
    indices_.clear();

    struct TangentAccum {
        glm::vec3 t = glm::vec3(0.0f);
        glm::vec3 b = glm::vec3(0.0f);
    };

    std::vector<TangentAccum> tangentAccum;

    for (auto& shape : shapes)
    {
        auto& mesh = shape.mesh;

        for (size_t i = 0; i < mesh.indices.size(); i += 3)
        {
            tinyobj::index_t i0 = mesh.indices[i + 0];
            tinyobj::index_t i1 = mesh.indices[i + 1];
            tinyobj::index_t i2 = mesh.indices[i + 2];

            glm::vec3 p0 = {
                attributes.vertices[3 * i0.vertex_index + 0],
                attributes.vertices[3 * i0.vertex_index + 1],
                attributes.vertices[3 * i0.vertex_index + 2]
            };

            glm::vec3 p1 = {
                attributes.vertices[3 * i1.vertex_index + 0],
                attributes.vertices[3 * i1.vertex_index + 1],
                attributes.vertices[3 * i1.vertex_index + 2]
            };

            glm::vec3 p2 = {
                attributes.vertices[3 * i2.vertex_index + 0],
                attributes.vertices[3 * i2.vertex_index + 1],
                attributes.vertices[3 * i2.vertex_index + 2]
            };

            glm::vec2 uv0 = (i0.texcoord_index >= 0) ?
                glm::vec2(attributes.texcoords[2 * i0.texcoord_index + 0],
                          attributes.texcoords[2 * i0.texcoord_index + 1]) :
                glm::vec2(0.0f);

            glm::vec2 uv1 = (i1.texcoord_index >= 0) ?
                glm::vec2(attributes.texcoords[2 * i1.texcoord_index + 0],
                          attributes.texcoords[2 * i1.texcoord_index + 1]) :
                glm::vec2(0.0f);

            glm::vec2 uv2 = (i2.texcoord_index >= 0) ?
                glm::vec2(attributes.texcoords[2 * i2.texcoord_index + 0],
                          attributes.texcoords[2 * i2.texcoord_index + 1]) :
                glm::vec2(0.0f);

            glm::vec3 n0 = {0,0,0};
            glm::vec3 n1 = {0,0,0};
            glm::vec3 n2 = {0,0,0};

            if (i0.normal_index >= 0)
                n0 = {
                    attributes.normals[3 * i0.normal_index + 0],
                    attributes.normals[3 * i0.normal_index + 1],
                    attributes.normals[3 * i0.normal_index + 2]
                };

            if (i1.normal_index >= 0)
                n1 = {
                    attributes.normals[3 * i1.normal_index + 0],
                    attributes.normals[3 * i1.normal_index + 1],
                    attributes.normals[3 * i1.normal_index + 2]
                };

            if (i2.normal_index >= 0)
                n2 = {
                    attributes.normals[3 * i2.normal_index + 0],
                    attributes.normals[3 * i2.normal_index + 1],
                    attributes.normals[3 * i2.normal_index + 2]
                };

            glm::vec3 edge1 = p1 - p0;
            glm::vec3 edge2 = p2 - p0;

            glm::vec2 dUV1 = uv1 - uv0;
            glm::vec2 dUV2 = uv2 - uv0;

            float denom = (dUV1.x * dUV2.y - dUV2.x * dUV1.y);

            glm::vec3 tangent(0.0f);

            if (fabs(denom) > 1e-6f)
            {
                float f = 1.0f / denom;

                tangent = f * (
                    dUV2.y * edge1 -
                    dUV1.y * edge2
                );
            }
          
            const unsigned int baseIndex = static_cast<unsigned int>(vertices_.size());

            vertices_.push_back({p0, n0, uv0, glm::vec4(tangent, 1.0f)});
            vertices_.push_back({p1, n1, uv1, glm::vec4(tangent, 1.0f)});
            vertices_.push_back({p2, n2, uv2, glm::vec4(tangent, 1.0f)});

            indices_.push_back(baseIndex + 0);
            indices_.push_back(baseIndex + 1);
            indices_.push_back(baseIndex + 2);
        }
    }

    for (auto& v : vertices_)
    {
        glm::vec3 N = glm::normalize(v.normal);
        glm::vec3 T = glm::normalize(glm::vec3(v.tangent));

        T = glm::normalize(T - N * glm::dot(N, T));

        v.tangent = glm::vec4(T, 1.0f);
    }

    initMesh();
}

  void Mesh::UploadToGPU()
  {
    if (vertices_.empty() || indices_.empty()) return;
    initMesh();
  }

  void Mesh::LoadOBJ_CPU(const char* path)
  {
    if (path == nullptr) return;
    LoadOBJ(path);
    UploadToGPU();
  }

  void Mesh::initMesh()
  {
    vertexArray_ = graphicsDevice_.createVertexArray();

    const bool isVulkan = dynamic_cast<VKDevice*>(&graphicsDevice_) != nullptr;

    if (!isVulkan) {
      vertexArray_->bind();
    }

    vertexBuffer_ = graphicsDevice_.createVertexBuffer(&vertices_[0], (unsigned int)vertices_.size() * sizeof(Vertex));
    vertexBuffer_->uploadData(&vertices_[0], (unsigned int)vertices_.size() * sizeof(Vertex));

    indexBuffer_ = graphicsDevice_.createIndexBuffer(&indices_[0], (unsigned int)indices_.size() * sizeof(unsigned int));
    indexBuffer_->uploadData(&indices_[0], (unsigned int)indices_.size() * sizeof(unsigned int));

    vertexArray_->addVertexBuffer(vertexBuffer_.get(), 0, 3, Float3, false, sizeof(Vertex), nullptr);
    vertexArray_->addVertexBuffer(vertexBuffer_.get(), 1, 3, Float3, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, normal)));
    vertexArray_->addVertexBuffer(vertexBuffer_.get(), 2, 2, Float2, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, texCoord)));
    vertexArray_->addVertexBuffer(vertexBuffer_.get(), 3, 4, Float4, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, tangent)));
    
    vertexArray_->setIndexBuffer(indexBuffer_.get());

    if (isVulkan) {

    }
    if (!isVulkan) {
      vertexArray_->unBind();
    }
  }

  std::shared_ptr<VertexArray> Mesh::vertexArray()
  {
    return vertexArray_;
  }

  u32 Mesh::indices()
  {
      return (u32)indices_.size();
  }
  
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
  

}