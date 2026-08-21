#include "ecs/probe_systems.hpp"

#include "ecs/light_probe.hpp"
#include "ecs/engine_components.hpp"
#include "ecs/world.hpp"
#include "ecs/system.hpp"
#include "render/api/probe_baker.hpp"
#include "core/camera.hpp"
#include "matsys/material.hpp"

namespace mam {

  void updateProbesSystem(Context& context) {
    auto* world = context.Get<World>();
    auto* baker = context.Get<ProbeBaker>();
    auto* lights = context.Get<LightList>();

    if (!world || !baker || !lights) return;

    baker->bake(*world, *lights);
  }

  void collectProbesSystem(Context& context) {
    auto* world = context.Get<World>();
    auto* probeList = context.Get<ProbeList>();

    if (!world || !probeList) return;

    probeList->clear();

    auto probeType = world->getComponentType<LightProbeComponent>();
    auto transformType = world->getComponentType<TransformComponent>();

    for (auto& arch : world->getArchetypes()) {
      if (!arch->signature().test(probeType))     continue;
      if (!arch->signature().test(transformType)) continue;

      auto* probeArray = arch->getArray<LightProbeComponent>(probeType);
      auto* trArray = arch->getArray<TransformComponent>(transformType);

      if (!probeArray || !trArray) continue;

      for (size_t i = 0; i < arch->getEntitiesNum(); ++i) {
        const auto& probe = probeArray->get(i);
        const auto& tr = trArray->get(i);

        if (probe.state != ProbeState::Baked) continue;

        GPUProbe gp{};
        gp.position_radius = { tr.position, probe.influenceRadius };

        constexpr float kProbeIntensity = 4.5f;
        for (int c = 0; c < kSHCoeffCount; ++c)
          gp.sh[c] = glm::vec4(probe.sh.coeffs[c] * kProbeIntensity, 0.f);

        probeList->probes.push_back(gp);
      }
    }
  }

  void uploadProbeToMaterialSystem(Context& context) {
    auto* probeList = context.Get<ProbeList>();
    auto* camera = context.Get<Camera>();
    auto* materialRegistry = context.Get<MaterialRegistry>();

    if (!probeList || !camera || !materialRegistry) return;

    const GPUProbe* best = nullptr;
    float           bestDist = std::numeric_limits<float>::max();
    const glm::vec3 camPos = camera->position();

    for (const auto& gp : probeList->probes) {
      const glm::vec3 pos = glm::vec3(gp.position_radius);
      const float     dist = glm::distance(camPos, pos);
      const float     radius = gp.position_radius.w;

      if (dist < radius && dist < bestDist) {
        bestDist = dist;
        best = &gp;
      }
    }

    if (!best) {
      for (const auto& gp : probeList->probes) {
        const float dist = glm::distance(camPos, glm::vec3(gp.position_radius));
        if (dist < bestDist) {
          bestDist = dist;
          best = &gp;
        }
      }
    }

    char uname[32];
    for (auto& [id, mat] : materialRegistry->getAllMaterials()) {
      if (mat->getName().find("pbr") == std::string::npos) continue;

      if (!best) {
        mat->setParameter("u_probeEnabled", 0);
        continue;
      }

      for (int i = 0; i < kSHCoeffCount; ++i) {
        std::snprintf(uname, sizeof(uname), "u_sh[%d]", i);
        mat->setParameter(uname, glm::vec3(best->sh[i]));
      }
      mat->setParameter("u_probeEnabled", 1);
    }
  }

} // namespace mam