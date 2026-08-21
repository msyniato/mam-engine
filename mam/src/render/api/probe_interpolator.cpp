#include "render/api/probe_interpolator.hpp"

#include <glm/glm.hpp>

namespace mam {

  void ProbeInterpolator::update(const std::vector<ProbeRef>& probes) {
    probes_ = probes;
  }

  float ProbeInterpolator::blendWeight(float dist,
                                       float influenceRadius,
                                       float blendDistance) {
    const float inner = influenceRadius - blendDistance;

    if (dist <= inner)           return 1.f;
    if (dist >= influenceRadius) return 0.f;

    float t = (dist - inner) / blendDistance;
    t = glm::clamp(t, 0.f, 1.f);
    return 1.f - (t * t * (3.f - 2.f * t));   
  }

  SHCoefficients ProbeInterpolator::interpolate(const glm::vec3& position) const {
    SHCoefficients result;
    float totalWeight = 0.f;

    for (const auto& p : probes_) {
      if (!p.sh) continue;

      float dist = glm::distance(position, p.position);
      float bw   = blendWeight(dist, p.influenceRadius, p.blendDistance);
      if (bw <= 0.f) continue;

      float idw = 1.f / (dist + 0.1f);
      float w   = bw * idw;

      for (int i = 0; i < kSHCoeffCount; ++i)
        result.coeffs[i] += p.sh->coeffs[i] * w;

      totalWeight += w;
    }

    if (totalWeight > 1e-6f) {
      for (int i = 0; i < kSHCoeffCount; ++i)
        result.coeffs[i] /= totalWeight;
    }

    return result;
  }

  glm::vec3 ProbeInterpolator::evaluateIrradiance(const glm::vec3& position,
                                                   const glm::vec3& normal) const {
    return interpolate(position).evaluate(normal);
  }

} // namespace mam
