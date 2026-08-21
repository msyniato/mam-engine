#include "render/api/probe_baker.hpp"

#include "ecs/world.hpp"
#include "ecs/engine_components.hpp"

#include <glm/gtc/constants.hpp>
#include <random>
#include <cmath>

namespace mam {

  static glm::vec3 uniformSphereDir(float u1, float u2) {
    const float theta = std::acos(1.f - 2.f * u1);
    const float phi = glm::two_pi<float>() * u2;
    const float sinT = std::sin(theta);
    return { sinT * std::cos(phi), sinT * std::sin(phi), std::cos(theta) };
  }

  static std::array<float, kSHCoeffCount> shBasis(const glm::vec3& d) {
    const float x = d.x;
    const float y = d.y;
    const float z = d.z;
    return {
       0.282095f,
       0.488603f * y,
       0.488603f * z,
       0.488603f * x,
       1.092548f * x * y,
       1.092548f * y * z,
       0.315392f * (3.f * z * z - 1.f),
       1.092548f * x * z,
       0.546274f * (x * x - y * y),
    };
  }

  void ProbeBaker::collectDirtyProbes(World& world) {
    const auto probeType = world.getComponentType<LightProbeComponent>();
    const auto transformType = world.getComponentType<TransformComponent>();

    for (auto& arch : world.getArchetypes()) {
      if (!arch->signature().test(probeType))     continue;
      if (!arch->signature().test(transformType)) continue;

      auto* probeArray = arch->getArray<LightProbeComponent>(probeType);
      auto* trArray = arch->getArray<TransformComponent>(transformType);

      if (!probeArray || !trArray) continue;

      for (size_t i = 0; i < arch->getEntitiesNum(); ++i) {
        auto& probe = probeArray->get(i);
        if (probe.state != ProbeState::NeedsBake) continue;

        BakeRequest req;
        req.position = trArray->get(i).position;
        req.priority = probe.bakePriority;
        req.probe = &probe;

        probe.state = ProbeState::Baking;
        queue_.push(req);
      }
    }
  }

  void ProbeBaker::bake(World& world, const LightList& lights) {
    collectDirtyProbes(world);
    if (queue_.empty()) return;

    int processed = 0;

    while (!queue_.empty() && processed < kMaxBakesPerFrame) {
      BakeRequest req = queue_.top();
      queue_.pop();

      req.probe->sh = projectToSH(req.position, lights);
      req.probe->state = ProbeState::Baked;

      ++processed;
    }
  }

  void ProbeBaker::invalidateAll(World& world) {
    while (!queue_.empty()) queue_.pop();

    const auto probeType = world.getComponentType<LightProbeComponent>();

    for (auto& arch : world.getArchetypes()) {
      if (!arch->signature().test(probeType)) continue;

      auto* probeArray = arch->getArray<LightProbeComponent>(probeType);
      if (!probeArray) continue;

      for (size_t i = 0; i < arch->getEntitiesNum(); ++i)
        probeArray->get(i).state = ProbeState::NeedsBake;
    }
  }

  SHCoefficients ProbeBaker::projectToSH(const glm::vec3& position,
    const LightList& lights) {
    SHCoefficients result{};

    const auto seed = static_cast<u32>(
      position.x * 73856093.f + position.y * 19349663.f + position.z * 83492791.f);
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(0.f, 1.f);

    const float weight = (4.f * glm::pi<float>()) / static_cast<float>(kSamples);

    for (int s = 0; s < kSamples; ++s) {
      const glm::vec3 dir = uniformSphereDir(dist(rng), dist(rng));
      const glm::vec3 radiance = sampleRadiance(position, dir, lights);
      const auto      basis = shBasis(dir);

      for (int c = 0; c < kSHCoeffCount; ++c)
        result.coeffs[c] += radiance * (basis[c] * weight);
    }

    return result;
  }

  glm::vec3 ProbeBaker::sampleRadiance(const glm::vec3& position,
    const glm::vec3& dir,
    const LightList& lights) {
    glm::vec3 total(0.f);
    glm::vec3 ambientAccum(0.f);

    for (const auto& gl : lights.lights) {
      ambientAccum += glm::vec3(gl.ambient) * 0.1f;

      if (gl.type == LightType::DirectionalLight) {
        const glm::vec3 lightDir = glm::normalize(-glm::vec3(gl.direction));
        const float     ndotl = glm::max(0.f, glm::dot(dir, lightDir));
        total += glm::vec3(gl.diffuse) * ndotl;
      }
      else {
        const glm::vec3 toLight = glm::vec3(gl.position) - position;
        const float     distToLight = glm::length(toLight);
        if (distToLight < 1e-4f) continue;

        const glm::vec3 lightDir = toLight / distToLight;
        const float     ndotl = glm::max(0.f, glm::dot(dir, lightDir));
        if (ndotl < 1e-5f) continue;

        const float atten = 1.f / (gl.constant_att
          + gl.linear_att * distToLight
          + gl.quadratic_att * distToLight * distToLight);
        total += glm::vec3(gl.diffuse) * (ndotl * atten);
      }
    }

    total += ambientAccum;

    return total;
  }

} // namespace mam