#include "ui/terrain_gen_panel.hpp"

#include <imgui.h>

namespace mam {
  

  void TerrainGenPanel::helpMarker(const char* desc)
  {
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
    {
      ImGui::BeginTooltip();
      ImGui::PushTextWrapPos(ImGui::GetFontSize() * 28.0f);
      ImGui::TextUnformatted(desc);
      ImGui::PopTextWrapPos();
      ImGui::EndTooltip();
    }
  }

  bool TerrainGenPanel::sectionHeader(const char* label, bool defaultOpen)
  {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth
                             | ImGuiTreeNodeFlags_Framed;
    if (defaultOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;
    return ImGui::TreeNodeEx(label, flags);
  }

  TerrainGenPanel::TerrainGenPanel()
  {

    std::strncpy(heightmapPathBuf_, params_.noise.heightmapPath.c_str(),  sizeof(heightmapPathBuf_) - 1);
    std::strncpy(splatPath0Buf_,    params_.splat.splatTexturePath0.c_str(), sizeof(splatPath0Buf_) - 1);
    std::strncpy(splatPath1Buf_,    params_.splat.splatTexturePath1.c_str(), sizeof(splatPath1Buf_) - 1);
    std::strncpy(splatPath2Buf_,    params_.splat.splatTexturePath2.c_str(), sizeof(splatPath2Buf_) - 1);
    
    std::strncpy(lsystemAxiomBuf_, params_.lsystem.axiom.c_str(), sizeof(lsystemAxiomBuf_) - 1);

    syncParamsToRuleEntries();
  }

  void TerrainGenPanel::syncParamsToRuleEntries()
  {
    ruleEntries_.clear();
    for (auto& [sym, expansion] : params_.lsystem.rules)
    {
      RuleEntry e{};
      e.symbol[0] = sym;
      std::strncpy(e.expansion, expansion.c_str(), sizeof(e.expansion) - 1);
      ruleEntries_.push_back(e);
    }

    if (ruleEntries_.empty())
    {
      RuleEntry e{};
      e.symbol[0] = 'X';
      std::strncpy(e.expansion, "F+[[X]-X]-F[-FX]+X", sizeof(e.expansion) - 1);
      ruleEntries_.push_back(e);
    }
  }

  void TerrainGenPanel::syncRuleEntriesToParams()
  {
    params_.lsystem.rules.clear();
    for (auto& entry : ruleEntries_)
    {
      if (entry.symbol[0] != '\0')
        params_.lsystem.rules[entry.symbol[0]] = entry.expansion;
    }
  }

  void TerrainGenPanel::render(bool* p_open)
  {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
    if (!ImGui::Begin("World Generation", p_open, flags))
    {
      ImGui::End();
      return;
    }

    dirty_ = false;
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    renderTerrainSection();
    ImGui::Spacing();

    renderVegetationSection();
    ImGui::Spacing();

    renderRegenerateButton();

    ImGui::End();
  }

  void TerrainGenPanel::renderTerrainSection()
  {
    if (!sectionHeader("Terrain")) return;
    
    if (ImGui::TreeNodeEx("Grid", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
    {
      auto& g = params_.grid;

      int chunksX = static_cast<int>(g.chunksX);
      int chunksZ = static_cast<int>(g.chunksZ);
      int subs    = static_cast<int>(g.subdivisions);
      float chunkSize = g.chunkSizeM;

      if (ImGui::DragInt("Chunks X##grid",  &chunksX, 1, 1, 64))  { g.chunksX = static_cast<u32>(chunksX); dirty_ = true; }
      helpMarker("Number of terrain chunks along the X axis.");

      if (ImGui::DragInt("Chunks Z##grid",  &chunksZ, 1, 1, 64))  { g.chunksZ = static_cast<u32>(chunksZ); dirty_ = true; }
      helpMarker("Number of terrain chunks along the Z axis.");

      if (ImGui::DragFloat("Chunk Size (m)##grid", &chunkSize, 1.f, 8.f, 2048.f, "%.0f m")) { g.chunkSizeM = chunkSize; dirty_ = true; }
      helpMarker("World-space side length of each chunk in metres.");

      if (ImGui::DragInt("Subdivisions##grid", &subs, 1, 2, 256)) { g.subdivisions = static_cast<u32>(subs); dirty_ = true; }
      helpMarker("Vertex grid resolution per chunk at the highest LOD. LODs 1-3 are auto-halved.");
      
      float totalX = g.chunksX * g.chunkSizeM;
      float totalZ = g.chunksZ * g.chunkSizeM;
      ImGui::TextDisabled("Total: %.0f x %.0f m", totalX, totalZ);

      ImGui::TreePop();
    }
    
    if (ImGui::TreeNodeEx("Noise & Height", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
    {
      auto& n = params_.noise;

      if (ImGui::DragFloat("Frequency##noise",     &n.frequency,      0.001f, 0.0001f, 1.0f, "%.4f")) dirty_ = true;
      helpMarker("Spatial frequency of the noise. Lower = broader features.");

      if (ImGui::DragFloat("Height Scale##noise",  &n.heightMult,     0.5f,   0.f,    2000.f, "%.1f m")) dirty_ = true;
      helpMarker("Multiplier applied to the raw [0,1] noise to produce world-space height.");

      if (ImGui::DragFloat("Persistence##noise",   &n.persistence,    0.01f,  0.0f,   1.0f,  "%.2f")) dirty_ = true;
      helpMarker("How quickly each successive octave drops in amplitude (0=flat, 1=equal weight).");

      if (ImGui::DragFloat("Redistribution##noise",&n.redistribution, 0.01f,  0.1f,   4.0f,  "%.2f")) dirty_ = true;
      helpMarker(">1 flattens lowlands and sharpens peaks; <1 inverts the effect.");

      int oct = n.octaves;
      if (ImGui::DragInt("Octaves##noise", &oct, 1, 1, 12)) { n.octaves = oct; dirty_ = true; }
      helpMarker("Number of fractal layers. More octaves = finer surface detail, higher cost.");

      n.heightmapPath = "../assets/textures/testheightmap.jpg";
      
      ImGui::TreePop();
    }
    
    ImGui::TreePop(); 
  }

  void TerrainGenPanel::renderVegetationSection()
  {
    if (!sectionHeader("Vegetation")) return;
    
    if (ImGui::TreeNodeEx("Scatter", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
    {
      auto& v = params_.vegetation;

      int count = static_cast<int>(v.count);
      if (ImGui::DragInt("Instance Count##veg", &count, 10, 0, 100000)) { v.count = static_cast<u32>(count); dirty_ = true; }
      helpMarker("Total number of tree instances placed across the terrain.");

      if (ImGui::DragFloat("Min Scale##veg",   &v.minScale, 0.1f, 0.1f, 50.f, "%.1f")) dirty_ = true;
      if (ImGui::DragFloat("Max Scale##veg",   &v.maxScale, 0.1f, 0.1f, 50.f, "%.1f")) dirty_ = true;
      helpMarker("Random uniform scale range applied per instance.");

      if (ImGui::DragFloat("Min Height##veg",  &v.minHeightToSpawn, 0.5f, 0.f, 1000.f, "%.1f m")) dirty_ = true;
      if (ImGui::DragFloat("Max Height##veg",  &v.maxHeightToSpawn, 0.5f, 0.f, 1000.f, "%.1f m")) dirty_ = true;
      helpMarker("Only spawn trees inside this world-space height range.");

      int seed = static_cast<int>(v.seed);
      if (ImGui::DragInt("Seed##veg", &seed, 1, 0, 99999)) { v.seed = static_cast<u32>(seed); dirty_ = true; }
      helpMarker("RNG seed for reproducible placement. Change to get a different scatter layout.");

      ImGui::TreePop();
    }
    
    if (ImGui::TreeNodeEx("L-System Tree Shape", ImGuiTreeNodeFlags_SpanAvailWidth))
    {
      auto& ls = params_.lsystem;

      ImGui::TextDisabled("Axiom");
      if (ImGui::InputText("##axiom", lsystemAxiomBuf_, sizeof(lsystemAxiomBuf_)))
      {
        ls.axiom = lsystemAxiomBuf_;
        dirty_ = true;
      }
      helpMarker("Starting string for the L-System expansion (e.g. \"X\", \"F\").");

      int iters = ls.iterations;
      if (ImGui::DragInt("Iterations##ls", &iters, 1, 1, 8)) { ls.iterations = iters; dirty_ = true; }
      helpMarker("Number of rewriting passes. More = more complex geometry, exponentially slower.");

      if (ImGui::DragFloat("Angle##ls",            &ls.angle,           0.5f,  1.f,  90.f,  "%.1f deg")) dirty_ = true;
      if (ImGui::DragFloat("Segment Length##ls",   &ls.segmentLength,   0.05f, 0.1f, 20.f,  "%.2f")) dirty_ = true;
      if (ImGui::DragFloat("Segment Radius##ls",   &ls.segmentRadius,   0.005f,0.01f, 2.f,  "%.3f")) dirty_ = true;
      if (ImGui::DragFloat("Length Decay##ls",     &ls.lengthDecay,     0.01f, 0.1f,  1.f,  "%.2f")) dirty_ = true;
      helpMarker("Each branch level's length = parent × decay.");
      if (ImGui::DragFloat("Radius Decay##ls",     &ls.radiusDecay,     0.01f, 0.1f,  1.f,  "%.2f")) dirty_ = true;
      int sides = ls.cylinderSides;
      if (ImGui::DragInt("Cylinder Sides##ls",     &sides, 1, 3, 16))  { ls.cylinderSides = sides; dirty_ = true; }
      helpMarker("Polygonal cross-section resolution of each branch cylinder.");
      
      ImGui::Spacing();
      ImGui::TextDisabled("Rewriting Rules  (symbol → expansion)");
      ImGui::Separator();

      for (int i = 0; i < static_cast<int>(ruleEntries_.size()); ++i)
      {
        auto& entry = ruleEntries_[i];
        ImGui::PushID(i);

        // Symbol (single char)
        ImGui::SetNextItemWidth(32.f);
        if (ImGui::InputText("##sym", entry.symbol, sizeof(entry.symbol)))
        {
          if (entry.symbol[1] != '\0') entry.symbol[1] = '\0';
          syncRuleEntriesToParams();
          dirty_ = true;
        }

        ImGui::SameLine();
        ImGui::TextUnformatted("→");
        ImGui::SameLine();

        float availW = ImGui::GetContentRegionAvail().x - 26.f;
        ImGui::SetNextItemWidth(availW > 10.f ? availW : 10.f);
        if (ImGui::InputText("##exp", entry.expansion, sizeof(entry.expansion)))
        {
          syncRuleEntriesToParams();
          dirty_ = true;
        }

        ImGui::SameLine();
        if (ImGui::SmallButton("X##del"))
        {
          ruleEntries_.erase(ruleEntries_.begin() + i);
          syncRuleEntriesToParams();
          dirty_ = true;
          ImGui::PopID();
          break; 
        }

        ImGui::PopID();
      }

      if (ImGui::SmallButton("+ Add Rule"))
      {
        RuleEntry e{};
        e.symbol[0] = 'A';
        ruleEntries_.push_back(e);
      }

      ImGui::TreePop();
    }

    ImGui::TreePop();
  }
  
  void TerrainGenPanel::renderRiverSection()
  {
    if (!sectionHeader("River")) return;
    
    if (ImGui::TreeNodeEx("Pathfinding", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
    {
      auto& r = params_.river;

      if (ImGui::DragFloat("Uphill Penalty##riv",    &r.uphillPenalty,    0.1f, 0.f, 50.f, "%.1f")) dirty_ = true;
      helpMarker("Extra cost the A* search incurs when moving to a higher cell. "
                 "Higher = rivers hug valleys more aggressively.");

      if (ImGui::DragFloat("Heuristic Weight##riv",  &r.heuristicWeight,  0.05f,0.5f,  5.f, "%.2f")) dirty_ = true;
      helpMarker(">1 speeds up search at the cost of optimality; 1 = true A*.");

      ImGui::TreePop();
    }
    
    if (ImGui::TreeNodeEx("Carving", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
    {
      auto& c = params_.riverCarve;

      if (ImGui::DragFloat("Carve Depth##riv",   &c.depth, 0.5f, 0.f, 100.f,  "%.1f m")) dirty_ = true;
      helpMarker("How many metres the terrain is depressed along the river bed.");

      if (ImGui::DragFloat("Carve Width##riv",   &c.width, 1.f,  1.f, 2000.f, "%.0f m")) dirty_ = true;
      helpMarker("Gaussian falloff half-width of the carved channel.");

      int stride = c.pathStride;
      if (ImGui::DragInt("Path Stride##riv", &stride, 1, 1, 16)) { c.pathStride = stride; dirty_ = true; }
      helpMarker("Sub-sampling stride of the height-field grid used for A*. "
                 "Higher = faster but coarser path.");

      ImGui::TreePop();
    }
    
    if (ImGui::TreeNodeEx("Water Surface", ImGuiTreeNodeFlags_SpanAvailWidth))
    {
      auto& w = params_.water;

      if (ImGui::DragFloat("Water Width##riv",  &w.waterWidth,  1.f, 1.f,  500.f, "%.0f m")) dirty_ = true;
      helpMarker("Visual width of the water mesh strip.");

      if (ImGui::DragFloat("Water Raise##riv",  &w.waterRaise,  0.1f,0.f, 50.f,  "%.1f m")) dirty_ = true;
      helpMarker("How far above the carved bed the water surface sits.");

      ImGui::TreePop();
    }

    ImGui::TreePop(); 
  }
  
  void TerrainGenPanel::renderRegenerateButton()
  {

    if (dirty_)
    {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.85f, 0.2f, 1.f));
      ImGui::TextUnformatted("  Parameters modified — press Regenerate to apply.");
      ImGui::PopStyleColor();
      ImGui::Spacing();
    }
    
    ImVec2 btnSize(ImGui::GetContentRegionAvail().x, 36.f);

    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.16f, 0.55f, 0.92f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.25f, 0.65f, 1.00f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.10f, 0.40f, 0.75f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_Text,           ImVec4(1.00f, 1.00f, 1.00f, 1.00f));

    bool clicked = ImGui::Button("  Regenerate World  ", btnSize);

    ImGui::PopStyleColor(4);

    if (clicked && callback_)
    {
      syncRuleEntriesToParams();
      params_.lsystem.axiom = lsystemAxiomBuf_;
      params_.noise.heightmapPath = heightmapPathBuf_;
      params_.splat.splatTexturePath0 = splatPath0Buf_;
      params_.splat.splatTexturePath1 = splatPath1Buf_;
      params_.splat.splatTexturePath2 = splatPath2Buf_;

      callback_(params_);
      dirty_ = false;
    }

    ImGui::Spacing();
    ImGui::TextDisabled("Warning: regeneration rebuilds all geometry and may take several seconds.");
  }

} // namespace mam
