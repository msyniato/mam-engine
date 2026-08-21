#pragma once

#include "common/common.hpp"

namespace mam {

  static constexpr int kSHCoeffCount = 9;

  /**
   * @brief L2 Spherical Harmonics coefficients for diffuse irradiance.
   *
   * Stores the baked irradiance of a light probe projected onto 9 SH
   * basis functions (Ramamoorthi & Hanrahan 2001).
   */
  struct SHCoefficients {
    std::array<glm::vec3, kSHCoeffCount> coeffs{};

    SHCoefficients() { coeffs.fill(glm::vec3(0.f)); }

    /**
     * @brief Evaluate irradiance for a given surface normal.
     * @param n Normalised surface normal in world space
     * @return Irradiance colour (linear, HDR)
     */
    glm::vec3 evaluate(const glm::vec3& n) const;
  };

  enum class ProbeState : u8 {
    Uninitialized = 0,
    NeedsBake,        ///< Dirty – will be queued on the next bake pass
    Baking,           ///< Currently being processed
    Baked,            ///< SH data is valid and ready for upload
  };

  /**
   * @brief Stores baked diffuse irradiance at a point in the scene.
   *
   * The probe's world-space position is taken from the entity's
   * TransformComponent.  Only probes with state == Baked contribute
   * to indirect lighting at runtime.
   */
  struct LightProbeComponent {
    float       influenceRadius = 10.f;   ///< Influence sphere radius (world units)
    float       blendDistance   = 2.f;    ///< Smooth blend zone at the sphere edge
    SHCoefficients sh;                    ///< Baked irradiance coefficients
    ProbeState  state           = ProbeState::NeedsBake;
    int         bakePriority    = 0;      ///< Lower = higher priority in the bake queue
    std::string debugName;                ///< Optional label for editor / serialisation
  };

  /**
   * @brief GPU-side representation of a light probe (std140 compatible).
   */
  struct alignas(16) GPUProbe {
    glm::vec4 position_radius;              ///< xyz = world position, w = influenceRadius
    glm::vec4 sh[kSHCoeffCount];            ///< xyz = SH coefficient, w = padding
  };

  /**
   * @brief Runtime list of GPU-ready probe data, analogous to LightList.
   */
  struct ProbeList {
    std::vector<GPUProbe> probes;

    void clear() { probes.clear(); }
  };

} // namespace mam
