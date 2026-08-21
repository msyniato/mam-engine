#include "render/api/vegetation.hpp"
#include "render/api/terrain.hpp"
#include "render/api/mesh.hpp"
#include "render/api/graphics_device.hpp"
#include "render/api/buffer.hpp"
#include "render/api/vertex_array.hpp"
#include "ecs/world.hpp"
#include "ecs/engine_components.hpp"
#include "matsys/material.hpp"

#include <stack>
#include <cmath>

namespace mam{
  LSystemTree::LSystemTree(const LSystemParams& params) : params_(params) {}

  std::string LSystemTree::generate() const
  {
    std::string current = params_.axiom;
     for (int i = 0; i < params_.iterations; ++i) {
       std::string next;
			 next.reserve(current.size() * 4); 
       for (char c : current) {
         auto it = params_.rules.find(c);
         if (it != params_.rules.end()) {
           next += it->second; 
         } else {
           next += c;
         }
       }
       current = std::move(next);
		 }

    return current;
  }

  void LSystemTree::buildMesh(const std::string& lsString, std::vector<Vertex>& outVerts, std::vector<unsigned int>& outIndices) const
  {
		const float a = glm::radians(params_.angle);

		LSystemState state;
    state.pos = glm::vec3(0.0f, 0.0f, 0.0f);   
    state.heading = glm::vec3(0.0f, 1.0f, 0.0f);   
    state.up = glm::vec3(0.0f, 0.0f, 1.0f);   
    state.left = glm::vec3(1.0f, 0.0f, 0.0f);
    state.length = params_.segmentLength;
    state.radius = params_.segmentRadius;

		std::stack<LSystemState> stateStack;

    for (char c : lsString) {
      switch(c) {
        case 'F': {
          glm::vec3 newPos = state.pos + state.heading * state.length;
					float r0 = state.radius;
					float r1 = state.radius * params_.radiusDecay;
          addCylinder(state.pos, newPos, r0, r1, outVerts, outIndices);
          state.pos = newPos;
          break;
        }
				case 'f': 
          state.pos += state.heading * state.length;
					break;
        case '+':
          state.heading = rotate(state.heading, state.up, a);
          state.left = rotate(state.left, state.up, a);
          break;
        case '-':
          state.heading = rotate(state.heading, state.up, -a);
          state.left = rotate(state.left, state.up, -a);
          break;
        case '&':
          state.heading = rotate(state.heading, state.left, a);
          state.up = rotate(state.up, state.left, a);
          break;
        case '^':
          state.heading = rotate(state.heading, state.left, -a);
          state.up = rotate(state.up, state.left, -a);
          break;
        case '\\':
          state.left = rotate(state.left, state.heading, a);
					state.up = rotate(state.up, state.heading, a);
          break;
        case '/':
          state.left = rotate(state.left, state.heading, -a);
					state.up = rotate(state.up, state.heading, -a);
          break;
        case '[':
          stateStack.push(state);
					state.length *= params_.lengthDecay;
					state.radius *= params_.radiusDecay;
          break;
        case ']':
          if (stateStack.empty()) {
            printf("[L-system] ERROR: ']' con stack vacia\n");
            break;
          }
          addLeaf(state, outVerts, outIndices);
          state = stateStack.top();
          stateStack.pop();
          break; 
        case 'L':
            addLeaf(state, outVerts, outIndices);
				  break;
        default:
					break;
			}
    }
  }

  void LSystemTree::addCylinder(const glm::vec3& from, const glm::vec3& to, float r0, float r1, std::vector<Vertex>& verts, std::vector<unsigned int>& indices) const
  {
		glm::vec3 dir = to - from;
		float length = glm::length(dir);

		if (length < 1e-5f) return;
		dir /= length;

		glm::vec3 arbitrary = (std::abs(dir.y) < 0.9f) ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);

		glm::vec3 u = glm::normalize(glm::cross(dir, arbitrary));
		glm::vec3 v = glm::cross(dir, u);

		const int N = params_.cylinderSides;
    const u32 base = static_cast<u32>(verts.size());

    for (int i = 0; i < N; ++i) {
      float theta = (i / static_cast<float>(N)) * 2.0f * glm::pi<float>();
      float cosTheta = std::cos(theta);
      float sinTheta = std::sin(theta);
      glm::vec3 n = glm::normalize(u * cosTheta + v * sinTheta);
      Vertex v0;
      v0.position = from + n * r0;
      v0.normal = n;
      v0.texCoord = {(float)i / N, 0.0f};
      v0.tangent = glm::vec4(dir, 1.0f);

      Vertex v1;
      v1.position = to + n * r1;
      v1.normal = n;
			v1.texCoord = { (float)i / N, 1.0f };
			v1.tangent = glm::vec4(dir, 1.0f);

      verts.push_back(v0);
			verts.push_back(v1);
    }

    for (int i = 0; i < N; ++i) {
			int next = (i + 1) % N;
      u32 a = base + static_cast<u32>(i)    * 2;
      u32 b = base + static_cast<u32>(i)    * 2 + 1;
      u32 c = base + static_cast<u32>(next) * 2;
      u32 d = base + static_cast<u32>(next) * 2 + 1;
      indices.push_back(a);
      indices.push_back(c);
      indices.push_back(b);
      indices.push_back(b);
      indices.push_back(c);
      indices.push_back(d);
		}
  }

  void LSystemTree::addLeaf(const LSystemState& lsState, std::vector<Vertex>& verts, std::vector<unsigned int>& indices) const
  {
		float hw = lsState.length * 1.2f;
		float hh = lsState.length * 1.8f;

		glm::vec3 right = glm::normalize(glm::cross(lsState.heading, lsState.up));
		glm::vec3 normal = glm::normalize(glm::cross(right, lsState.heading));

		glm::vec3 p0 = lsState.pos - right * hw;
		glm::vec3 p1 = lsState.pos + right * hw;
    glm::vec3 p2 = lsState.pos + right * hw + lsState.heading * hh * 2.0f;
    glm::vec3 p3 = lsState.pos - right * hw + lsState.heading * hh * 2.0f;

		u32 base = static_cast<u32>(verts.size());

    auto makeLeafVertex = [&](const glm::vec3& pos, const glm::vec2& texCoord) {
      Vertex v;
      v.position = pos;
      v.normal = normal;
      v.texCoord = texCoord;
      v.tangent = glm::vec4(right, 1.0f);
      return v;
		};

		verts.push_back(makeLeafVertex(p0, { 0.0f, 0.0f }));
		verts.push_back(makeLeafVertex(p1, { 1.0f, 0.0f }));
		verts.push_back(makeLeafVertex(p2, { 1.0f, 1.0f }));
		verts.push_back(makeLeafVertex(p3, { 0.0f, 1.0f }));

		indices.push_back(base);
		indices.push_back(base + 1);
		indices.push_back(base + 2);
		indices.push_back(base);
		indices.push_back(base + 2);
		indices.push_back(base + 3);

    indices.push_back(base);
		indices.push_back(base + 2);
		indices.push_back(base + 1);
		indices.push_back(base);
		indices.push_back(base + 3);
		indices.push_back(base + 2);
  }

  glm::vec3 LSystemTree::rotate(const glm::vec3& vec, const glm::vec3& axis, float radians)
  {
		float c = std::cos(radians);
		float s = std::sin(radians);

		return vec * c + glm::cross(axis, vec) * s + axis * glm::dot(axis, vec) * (1.0f - c);
  }

  VegetationInstancer::VegetationInstancer(GraphicsDevice& gd, const Terrain& terrain, const LSystemParams& params, const VegetationParams& vegParams)
  {
    init(gd, terrain, params, vegParams);
  }

  void VegetationInstancer::init(GraphicsDevice& gd, const Terrain& terrain, const LSystemParams& params, const VegetationParams& vegParams)
  {
    vegParams_ = vegParams;

    LSystemTree tree(params);
    std::string ls = tree.generate();

    std::vector<Vertex> treeVerts;
    std::vector<unsigned int> treeIndices;
    tree.buildMesh(ls, treeVerts, treeIndices);

    treeMesh_ = std::make_unique<Mesh>(gd, "LSystemTree", treeVerts, treeIndices);

    std::mt19937 rng(vegParams.seed);
    std::uniform_real_distribution<float> distX(0.0f, terrain.totalSizeX());
    std::uniform_real_distribution<float> distZ(0.0f, terrain.totalSizeZ());
    std::uniform_real_distribution<float> distScale(vegParams.minScale, vegParams.maxScale);
    std::uniform_real_distribution<float> distYaw(0.0f, 360.0f);

    std::vector<glm::mat4> matrices;
    matrices.reserve(static_cast<size_t>(vegParams.count) * 6);
    u32 maxAttempts = vegParams.count * 6;

    for (u32 attempt = 0; attempt < maxAttempts; ++attempt) {
      float x = distX(rng);
      float z = distZ(rng);
      float y = terrain.getHeightAt(x, z);

      if (y < vegParams.minHeightToSpawn || y > vegParams.maxHeightToSpawn) {
        continue;
      }

      if (terrain.riverMaskAt(x, z) > 0.15f) {
        continue;
      }

      glm::vec3 pos = { x, y, z };
      float scale = distScale(rng);
      float yaw = glm::radians(distYaw(rng));

      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, pos);
      model = glm::rotate(model, yaw, { 0.0f, 1.0f, 0.0f });
      model = glm::scale(model, glm::vec3(scale));
      matrices.push_back(model);
    }

    instanceCount_ = static_cast<u32>(matrices.size());

    u32 bufferSize = instanceCount_ * static_cast<u32>(sizeof(glm::mat4));
    instancerBuffer_ = gd.createVertexBuffer(bufferSize);
    instancerBuffer_->uploadData(matrices.data(), bufferSize, 0);

    treeMesh_->vertexArray()->bind();
    treeMesh_->vertexArray()->addInstanceBufferMat4(instancerBuffer_.get(), 4, 1);
    treeMesh_->vertexArray()->unBind();
  }

  void VegetationInstancer::repositionOnTerrain(GraphicsDevice& gd, const Terrain& terrain, World& world)
  {
    if (!instancerBuffer_ || instanceCount_ == 0) return;

    std::mt19937 rng(vegParams_.seed);
    std::uniform_real_distribution<float> distX(0.0f, terrain.totalSizeX());
    std::uniform_real_distribution<float> distZ(0.0f, terrain.totalSizeZ());
    std::uniform_real_distribution<float> distScale(vegParams_.minScale, vegParams_.maxScale);
    std::uniform_real_distribution<float> distYaw(0.0f, 360.0f);

    std::vector<glm::mat4> matrices;
    matrices.reserve(static_cast<size_t>(vegParams_.count) * 6);
    u32 maxAttempts = vegParams_.count * 6;

    for (u32 attempt = 0; attempt < maxAttempts; ++attempt) {
      float x = distX(rng);
      float z = distZ(rng);
      float y = terrain.getHeightAt(x, z);

      if (y < vegParams_.minHeightToSpawn || y > vegParams_.maxHeightToSpawn) continue;
      if (terrain.riverMaskAt(x, z) > 0.15f) continue;

      float scale = distScale(rng);
      float yaw   = glm::radians(distYaw(rng));

      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, { x, y, z });
      model = glm::rotate(model, yaw, { 0.0f, 1.0f, 0.0f });
      model = glm::scale(model, glm::vec3(scale));
      matrices.push_back(model);
    }

    instanceCount_ = static_cast<u32>(matrices.size());
    u32 bufferSize = instanceCount_ * static_cast<u32>(sizeof(glm::mat4));
    instancerBuffer_->uploadData(matrices.data(), bufferSize, 0);

    auto* ic = world.getComponent<InstanceComponent>(entity_);
    if (ic) ic->instanceCount = instanceCount_;
  }

  void VegetationInstancer::clear(World& world)
  {
    if (entity_) {
       world.destroyEntity(entity_);
       entity_ = kInvalidEntity;
		}
		instanceCount_ = 0;
  }

  void VegetationInstancer::registerEntitiesInWorld(World& world, Material* material)
  {
    if(!treeMesh_ || instanceCount_ == 0)
      return;

		Entity e = world.createEntity();
		entity_ = e;

    RenderComponent rc;
		rc.mesh = treeMesh_.get();
		rc.material = material;
		world.addComponent<RenderComponent>(e, rc);

		auto tc = world.getComponent<TransformComponent>(e);
    if(tc)
    {
      tc->position = glm::vec3(0.0f);
      tc->rotation = glm::vec3(0.0f);
			tc->scale = glm::vec3(1.0f);
    }

		InstanceComponent ic;
		ic.instanceBuffer = instancerBuffer_.get();
		ic.instanceCount = instanceCount_;
		world.addComponent<InstanceComponent>(e, ic);
  }

  VegetationInstancer& VegetationInstancer::operator=(VegetationInstancer&& other)
  {
    if (this != &other) {
      treeMesh_ = std::move(other.treeMesh_);
      instancerBuffer_ = std::move(other.instancerBuffer_);
      instanceCount_ = other.instanceCount_;
      entity_ = other.entity_;
      other.instanceCount_ = 0;
      other.entity_ = kInvalidEntity;
    }
		return *this;
  }

} //namespace mam