#pragma once

#include "common/common.hpp"
#include "render/api/terrain.hpp"
#include "render/api/vegetation.hpp"
#include "render/api/river.hpp"

#include <imgui.h>
#include <functional>

namespace mam {

  /**
   * @brief All parameters bundled into one struct for the regenerate callback.
   */
  struct WorldGenParams {
    TerrainGridParams  grid;
    TerrainNoiseParams noise;
    TerrainSplatParams splat;
    LSystemParams      lsystem;
    VegetationParams   vegetation;
    RiverParams        river;
    RiverCarveParams   riverCarve;
    WaterMeshParams    water;
  };

  /**
   * @brief Callback signature invoked when the user clicks "Regenerate".
   *
   * The host application should wire this up to re-run
   * Terrain::init(), VegetationInstancer::init() and the river build step.
   */
  using WorldGenCallback = std::function<void(const WorldGenParams&)>;

  /**
   * @brief ImGui panel that exposes all terrain / vegetation / river
   *        parameters and a single "Regenerate World" button.
   *
   * Usage:
   *   // In your app init
   *   panel_.setCallback([&](const WorldGenParams& p){
   *       terrain_.init(gd, p.grid, p.noise, p.splat, js);
   *       vegetation_.init(gd, terrain_, p.lsystem, p.vegetation);
   *       // river is handled inside Terrain::init via River::build
   *   });
   *
   *   // In your ImGui frame
   *   panel_.render();
   */
  class TerrainGenPanel {
  public:
    TerrainGenPanel();
    ~TerrainGenPanel() = default;

    /**
     * @brief Register the callback that executes the actual regeneration.
     */
    void setCallback(WorldGenCallback cb) { callback_ = std::move(cb); }

    /**
     * @brief Seed the panel with the current live params so the UI reflects
     *        what is already in the world.
     */
    void setParams(const WorldGenParams& p) { params_ = p; }

    /** Read back whatever is currently shown in the panel. */
    const WorldGenParams& params() const { return params_; }

    /**
     * @brief Render the panel inside the currently open ImGui window (or
     *        call ImGui::Begin/End around it yourself).
     *
     * Typically called from ImGUIWrapper::render() after a
     *   if (showTerrainPanel_) renderTerrainGenPanel(context);
     * block, following the pattern of the other panels.
     */
    void render(bool* p_open = nullptr);

  private:
    // ── section helpers ──────────────────────────────────────────────────
    void renderTerrainSection();
    void renderVegetationSection();
    void renderRiverSection();
    void renderRegenerateButton();

    // ── helpers ──────────────────────────────────────────────────────────
    static void helpMarker(const char* desc);
    static bool sectionHeader(const char* label, bool defaultOpen = true);

    // ── state ────────────────────────────────────────────────────────────
    WorldGenParams   params_;
    WorldGenCallback callback_;

    bool dirty_ = false; ///< True when any param was changed this frame.

    // Char buffers for string fields (ImGui::InputText needs mutable arrays)
    char heightmapPathBuf_[256]{};
    char splatPath0Buf_[256]{};
    char splatPath1Buf_[256]{};
    char splatPath2Buf_[256]{};
    char lsystemAxiomBuf_[64]{};

    // L-System rule buffers – keyed by symbol (we only expose a few common ones)
    struct RuleEntry { char symbol[4]{}; char expansion[128]{}; };
    std::vector<RuleEntry> ruleEntries_;

    void syncRuleEntriesToParams();
    void syncParamsToRuleEntries();
  };

} // namespace mam
