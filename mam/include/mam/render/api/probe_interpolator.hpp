#pragma once

#include "common/common.hpp"
#include "ecs/light_probe.hpp"

namespace mam {

  /**
   * @brief Blends the irradiance of multiple baked probes at a query position.
   *
   * Uses inverse-distance weighting combined with a smooth-step falloff over
   * each probe's blend zone.  Call update() once per frame with the current
   * set of baked probes, then evaluateIrradiance() wherever needed (e.g. for
   * CPU-lit particles or character shading).
   */
  class ProbeInterpolator {
  public:

    /**
     * @brief Lightweight non-owning reference to a baked probe.
     */
    struct ProbeRef {
      glm::vec3            position;
      float                influenceRadius;
      float                blendDistance;
      const SHCoefficients* sh;   ///< Non-owning pointer into the component
    };

    /**
     * @brief Rebuild the internal probe list from the current frame's data.
     * @param probes List of references to baked probes
     */
    void update(const std::vector<ProbeRef>& probes);

    /**
     * @brief Interpolate SH coefficients at a world-space position.
     * @param position Query position
     * @return Blended SH coefficients (zero-initialised if no probe is in range)
     */
    SHCoefficients interpolate(const glm::vec3& position) const;

    /**
     * @brief Evaluate irradiance at 'position' for a surface with normal 'n'.
     * @param position Query position
     * @param normal   Surface normal (world space, normalised)
     * @return Linear HDR irradiance colour
     */
    glm::vec3 evaluateIrradiance(const glm::vec3& position,
                                  const glm::vec3& normal) const;

  private:

    std::vector<ProbeRef> probes_;

    /**
     * @brief Smooth blend weight: 1 at the probe centre, 0 at influenceRadius.
     * @param dist            Distance from the probe centre
     * @param influenceRadius Outer radius of influence
     * @param blendDistance   Width of the smooth transition zone
     */
    static float blendWeight(float dist,
                             float influenceRadius,
                             float blendDistance);
  };

} // namespace mam
