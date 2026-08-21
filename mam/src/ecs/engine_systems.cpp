#include "ecs/engine_systems.hpp"
#include "ecs/world.hpp"
#include "ecs/engine_components.hpp"
#include "render/api/graphics_device.hpp"
#include "render/api/buffer.hpp"
#include "matsys/material.hpp"
#include "core/camera.hpp"
#include "core/utils.hpp"
#include "ecs/system.hpp"
#include "render/api/renderer.hpp"
#include "render/api/terrain.hpp"
#include "jobsys/job_system.hpp"

namespace mam {
  
  struct FrustumPlane {
    glm::vec3 normal;
    float     d;
  };

  static std::array<FrustumPlane, 6> extractFrustumPlanes(const glm::mat4& vp)
  {
    glm::vec4 row0 = { vp[0][0], vp[1][0], vp[2][0], vp[3][0] };
    glm::vec4 row1 = { vp[0][1], vp[1][1], vp[2][1], vp[3][1] };
    glm::vec4 row2 = { vp[0][2], vp[1][2], vp[2][2], vp[3][2] };
    glm::vec4 row3 = { vp[0][3], vp[1][3], vp[2][3], vp[3][3] };

    auto makePlane = [](glm::vec4 v) -> FrustumPlane {
      float len = glm::length(glm::vec3(v));
      return { glm::vec3(v) / len, v.w / len };
      };

    return {
      makePlane(row3 + row0), // Left
      makePlane(row3 - row0), // Right
      makePlane(row3 + row1), // Bottom
      makePlane(row3 - row1), // Top
      makePlane(row3 + row2), // Near
      makePlane(row3 - row2), // Far
    };
  }

  static bool isAABBVisible(const glm::vec3& min, const glm::vec3& max,
    const std::array<FrustumPlane, 6>& planes)
  {
    for (const auto& p : planes) {
      glm::vec3 pv = {
          p.normal.x >= 0.f ? max.x : min.x,
          p.normal.y >= 0.f ? max.y : min.y,
          p.normal.z >= 0.f ? max.z : min.z,
      };
      if (glm::dot(p.normal, pv) + p.d < 0.f) return false;
    }
    return true;
  }

  static bool isSphereVisible(const glm::vec3& center, float radius,
    const std::array<FrustumPlane, 6>& planes)
  {
    for (const auto& p : planes)
      if (glm::dot(p.normal, center) + p.d < -radius) return false;
    return true;
  }

  void updateRenderSystem(Context& context) {
    auto* queue = context.Get<RenderQueue>();
    auto* world = context.Get<World>();
    auto* camera = context.Get<Camera>();

    glm::mat4 view = glm::lookAt(camera->position(),
      camera->position() + camera->front(), camera->up());
    glm::mat4 projection = glm::perspective(glm::radians(camera->fov()),
      camera->aspectRatio(), camera->zNear(), camera->zFar());
    auto frustumPlanes = extractFrustumPlanes(projection * view);

    auto transformType = world->getComponentType<TransformComponent>();
    auto renderType = world->getComponentType<RenderComponent>();
    auto lodType = world->getComponentType<LODComponent>();
    auto instanceType = world->getComponentType<InstanceComponent>();

    for (auto& arch : world->getArchetypes()) {
      if (!arch->signature().test(transformType)) continue;
      if (!arch->signature().test(renderType))    continue;

      bool hasLOD = arch->signature().test(lodType);
      bool hasInstance = arch->signature().test(instanceType);

      auto* lods = hasLOD ? arch->getArray<LODComponent>(lodType) : nullptr;
      auto* instances = hasInstance ? arch->getArray<InstanceComponent>(instanceType) : nullptr;
      auto* transforms = arch->getArray<TransformComponent>(transformType);
      auto* renders = arch->getArray<RenderComponent>(renderType);

      for (size_t i = 0; i < arch->getEntitiesNum(); ++i) {
        auto& rc = renders->get(i);
        auto& tc = transforms->get(i);
        if (!rc.mesh || !rc.material) continue;

        if (rc.boundingSphereRadius > 0.f)
          if (!isSphereVisible(rc.boundingSphereCenter, rc.boundingSphereRadius, frustumPlanes))
            continue;
        if (rc.aabbMin != rc.aabbMax)
          if (!isAABBVisible(rc.aabbMin, rc.aabbMax, frustumPlanes))
            continue;

        Mesh* selectedMesh = rc.mesh;
        if (lods) {
          auto& lod = lods->get(i);
          float dist = glm::distance(camera->position(), lod.center);
          selectedMesh = lod.meshes[lod.levels - 1];
          for (int l = 0; l < lod.levels; ++l) {
            if (dist < lod.distances[l]) {
              selectedMesh = lod.meshes[l];
              break;
            }
          }
        }

        if (instances) {
          auto& inst = instances->get(i);
          if (inst.instanceBuffer && inst.instanceCount > 0) {
            RenderCommandInstanced cmd;
            cmd.mesh           = selectedMesh;
            cmd.material       = rc.material;
            cmd.instanceBuffer = inst.instanceBuffer;
            cmd.instanceCount  = inst.instanceCount;
            cmd.textures       = rc.textures;
            cmd.layer          = RenderLayer::Opaque;
            queue->submitInstanced(cmd);
          }
        }
        else {
          RenderCommand cmd;
          cmd.mesh         = selectedMesh;
          cmd.material     = rc.material;
          cmd.transform    = getModel(tc.position, tc.rotation, tc.scale);
          cmd.textures     = rc.textures;
          cmd.layer        = RenderLayer::Opaque;
          cmd.useSplatting = rc.useSplatting;
          queue->submit(cmd);
        }
      }
    }
  }
  
  void collectLightsSystem(Context& context) {
    
    auto* world     = context.Get<World>();
    auto* lightList = context.Get<LightList>();
    
    if (!lightList) return;

    lightList->lights.clear();
    
    auto lightType = world->getComponentType<LightComponent>();
    auto trType = world->getComponentType<TransformComponent>();
    
    for (auto& arch : world->getArchetypes()) {
      if (!arch->signature().test(lightType)) continue;
      if (!arch->signature().test(trType)) continue;
        
      auto* lightArray = arch->getArray<LightComponent>(lightType);
      auto* trArray = arch->getArray<TransformComponent>(trType);
        
      for (size_t i = 0; i < arch->getEntitiesNum(); ++i) {
        auto& lc = lightArray->get(i);
        auto& tr = trArray->get(i);
            
            
        GPULight gl{};
        
        gl.position = tr.position;     
        gl.constant_att = lc.constant_att; 
        
        gl.ambient = lc.ambient;      
        gl.linear_att = lc.linear_att;     
        
        gl.diffuse = lc.diffuse;      
        gl.quadratic_att = lc.quadratic_att;  
        
        gl.specular = lc.specular;     
        gl.cut_off = lc.cut_off;        
        
        gl.direction = lc.direction;    
        gl.outer_cut_off = lc.outer_cut_off;  
        
        gl.type = lc.type;
        gl.pad  = glm::vec3(-1.0f, 0.0f, 0.0f);
        
        lightList->lights.push_back(gl);
      }
    }
  }

} //namespace mam
