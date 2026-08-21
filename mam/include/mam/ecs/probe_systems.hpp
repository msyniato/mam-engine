#pragma once

#include "ecs/system.hpp"

namespace mam {

  /**
   * @brief Bake dirty light probes using the current analytic LightList.
   *
   * Requires in Context: World*, ProbeBaker*, LightList*
   * Run after collectLightsSystem, before collectProbesSystem.
   */
  void updateProbesSystem(Context& context);

  /**
   * @brief Collect all baked probes into ProbeList for GPU upload.
   *
   * Requires in Context: World*, ProbeList*
   * Run after updateProbesSystem, before render_system.
   */
  void collectProbesSystem(Context& context);

  /**
   * @brief Upload the nearest probe's SH coefficients to every PBR material.
   *
   * Finds the closest baked probe to the camera position and sets
   * u_sh[0..8] and u_probeEnabled on all materials in the MaterialRegistry.
   *
   * Requires in Context: ProbeList*, Camera*, MaterialRegistry*
   * Run after collectProbesSystem, before render_system.
   */
  void uploadProbeToMaterialSystem(Context& context);

} // namespace mam
