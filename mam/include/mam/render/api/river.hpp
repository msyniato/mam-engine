#pragma once

#include "common/common.hpp"

namespace mam {

  /**
   * @brief Parameters controlling the A* path-finding cost for river routing.
   */
  struct RiverParams {
    float uphillPenalty   = 8.0f; ///< Extra cost assigned to uphill moves during path search.
    float heuristicWeight = 1.0f; ///< A* heuristic weight (1.0 = standard admissible heuristic).
  };

  /**
   * @brief Parameters controlling the physical carving of the river channel.
   */
  struct RiverCarveParams {
    float depth      = 30.0f; ///< Depth of the carved riverbed below the original surface.
    float width      = 450.0f; ///< Half-width of the carved channel in world units.
    int   pathStride = 4;      ///< Sampling stride along the path for carve kernel passes.
  };

  /**
   * @brief Output data produced by the river builder.
   */
  struct RiverResult {
    std::vector<glm::ivec2> path; ///< Sequence of grid cells forming the river path.
    std::vector<float>      bed;  ///< Carved height values along the path.
    std::vector<float>      mask; ///< Per-cell influence mask in [0, 1].
  };

  /**
   * @brief Parameters for constructing the water surface mesh.
   */
  struct WaterMeshParams {
    float waterWidth = 200.0f; ///< Half-width of the water surface quad strip in world units.
    float waterRaise = 4.0f;   ///< Height offset above the riverbed for the water surface.
  };

  /**
   * @brief Build a water surface mesh along a river path.
   * @param path       Grid-cell path produced by the river builder.
   * @param bed        Carved bed heights along the path.
   * @param cellSize   World-space size of one grid cell.
   * @param wp         Water mesh parameters.
   * @param outVerts   Output vertex buffer (appended to).
   * @param outIndices Output index buffer (appended to).
   */
  void buildWaterMesh(const std::vector<glm::ivec2>& path,
                      const std::vector<float>& bed,
                      float cellSize,
                      const WaterMeshParams& wp,
                      std::vector<Vertex>& outVerts,
                      std::vector<unsigned int>& outIndices);

  /**
   * @brief Compute a river path and carve it into a flat height field.
   * @param field    Height field array modified in place.
   * @param w        Width of the height field in cells.
   * @param h        Height of the height field in cells.
   * @param source   Starting grid cell (river source).
   * @param sink     Ending grid cell (river mouth).
   * @param params   Path-finding parameters.
   * @param carve    Channel carving parameters.
   * @param cellSize World-space size of one grid cell.
   * @return RiverResult containing the path, bed heights, and influence mask.
   */
  RiverResult buildRiver(std::vector<float>& field, int w, int h,
                         glm::ivec2 source, glm::ivec2 sink,
                         const RiverParams& params,
                         const RiverCarveParams& carve,
                         float cellSize);

  /**
   * @brief Stateful river subsystem operating on a raw height-field grid.
   *
   * Independent of the Terrain class. Retains the computed path, bed heights,
   * and influence mask so downstream systems (water mesh, vegetation masking)
   * can query them without recomputation.
   */
  class River {
  public:
    River() = default;

    /**
     * @brief Construct with specific path-finding and carving parameters.
     * @param p Path-finding parameters.
     * @param c Channel carving parameters.
     */
    River(const RiverParams& p, const RiverCarveParams& c)
      : params_(p), carve_(c) {}

    /**
     * @brief Replace the path-finding parameters.
     * @param p New parameters.
     */
    void setParams(const RiverParams& p) { params_ = p; }

    /**
     * @brief Replace the channel carving parameters.
     * @param c New parameters.
     */
    void setCarveParams(const RiverCarveParams& c) { carve_ = c; }

    /**
     * @brief Compute and carve a river into the given height field.
     * @param field    Height field to modify.
     * @param w        Field width in cells.
     * @param h        Field height in cells.
     * @param source   Source grid cell.
     * @param sink     Sink (mouth) grid cell.
     * @param cellSize World-space cell size.
     * @return True if a valid path was found and carved.
     */
    bool build(std::vector<float>& field, int w, int h,
               glm::ivec2 source, glm::ivec2 sink, float cellSize);

    /// Returns true if no river path has been computed yet.
    bool empty() const { return path_.empty(); }
    /// Returns the computed river path as a sequence of grid cells.
    const std::vector<glm::ivec2>& path() const { return path_; }
    /// Returns the carved bed heights along the path.
    const std::vector<float>& bed()  const { return bed_; }
    /// Returns the per-cell influence mask.
    const std::vector<float>& mask() const { return mask_; }

  private:
    RiverParams             params_{};
    RiverCarveParams        carve_{};
    std::vector<glm::ivec2> path_;
    std::vector<float>      bed_;
    std::vector<float>      mask_;
  };

} // namespace mam
