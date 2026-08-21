#pragma once

#include "common/common.hpp"
#include "ecs/light_probe.hpp"

namespace mam {

  class World;
  struct LightList;

  struct BakeRequest {
    LightProbeComponent* probe;
    glm::vec3 position;
    int       priority;

    /// Min-heap ordering: lower priority value = processed first
    bool operator>(const BakeRequest& o) const { return priority > o.priority; }
  };

  /**
   * @brief Schedules and executes the CPU-side baking of LightProbeComponents.
   *
   * Each frame, bake() processes up to kMaxBakesPerFrame probes from the
   * internal priority queue.  Irradiance is sampled from the analytic lights
   * already collected in LightList and projected into SH2 coefficients.
   *
   * Usage:
   *   - Keep one instance alive alongside the World (e.g. in the Context).
   *   - Call bake() once per frame after collectLightsSystem has run.
   *   - Call invalidateAll() whenever a dynamic light moves.
   */
  class ProbeBaker {
  public:

    /// Probes baked per frame (budget control)
    static constexpr int kMaxBakesPerFrame = 4;

    /// Monte-Carlo samples for the SH projection
    static constexpr int kSamples = 512;

    /**
     * @brief Process up to kMaxBakesPerFrame dirty probes.
     * @param world  Active world (component arrays accessed directly)
     * @param lights Analytic light list from collectLightsSystem
     */
    void bake(World& world, const LightList& lights);

    /**
     * @brief Mark every probe in the world as NeedsBake.
     *
     * Call when a dynamic light moves or the scene changes significantly.
     * @param world Active world
     */
    void invalidateAll(World& world);

    /// Number of probes still pending in the queue
    size_t pendingCount() const { return queue_.size(); }

  private:

    std::priority_queue<BakeRequest,
                        std::vector<BakeRequest>,
                        std::greater<BakeRequest>> queue_;

    /// Collect probes in NeedsBake state and push them onto the queue
    void collectDirtyProbes(World& world);

    /**
     * @brief Project irradiance at 'position' into SH2 coefficients.
     * @param position Probe world-space origin
     * @param lights   Analytic light list
     */
    SHCoefficients projectToSH(const glm::vec3& position,
                               const LightList& lights);

    /**
     * @brief Evaluate radiance arriving at 'position' from direction 'dir'.
     * @param position Probe world-space origin
     * @param dir      Sample direction (unit vector)
     * @param lights   Analytic light list
     */
    glm::vec3 sampleRadiance(const glm::vec3& position,
                             const glm::vec3& dir,
                             const LightList& lights);
  };

} // namespace mam
