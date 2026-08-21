#include "ui/imgui_wrapper.hpp"

#include "ecs/world.hpp"
#include "ecs/engine_components.hpp"

#include <imgui.h>

namespace mam {

  static std::string dropTarget(
      const char* label,
      const char* payloadType,
      const std::string& currentValue)
  {
    std::string result = "";

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));

    ImGui::BeginChild(
        label,
        ImVec2(ImGui::GetContentRegionAvail().x, 28),
        true,
        ImGuiWindowFlags_NoScrollbar
    );

    if (currentValue.empty() || currentValue == "None") {
      ImGui::TextDisabled("  Drop %s here...", label);
    } else {
      std::filesystem::path p(currentValue);
      ImGui::Text("  %s", p.filename().string().c_str());

      float clearWidth = 20.0f;
      ImGui::SameLine(ImGui::GetContentRegionAvail().x - clearWidth);
      ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0,0,0,0));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 0.6f));
      if (ImGui::SmallButton("x"))
        result = "CLEARED";
      ImGui::PopStyleColor(2);
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadType)) {
        result = std::string(static_cast<const char*>(payload->Data));
      }
      ImGui::EndDragDropTarget();
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
      ImDrawList* dl = ImGui::GetWindowDrawList();
      ImVec2 min = ImGui::GetItemRectMin();
      ImVec2 max = ImGui::GetItemRectMax();
      dl->AddRect(min, max, IM_COL32(100, 180, 255, 200), 4.0f, 0, 2.0f);
    }

    return result;
  }

  void InspectorPanel::render(World* world) {
    if (selectedEntity_ == kInvalidEntity) {
        ImGui::TextDisabled("No entity selected");
        return;
    }

    auto handle = world->getHandle(selectedEntity_);

    ImGui::Text("%s (Entity %u)", handle.name().c_str(), handle.entity());
    ImGui::Separator();

    auto* tc = world->getComponent<TransformComponent>(selectedEntity_);
    auto* rc = world->getComponent<RenderComponent>(selectedEntity_);

    if (tc) renderTransformComponent(tc);
    if (rc) renderRenderComponent(rc, world);
  }

  void InspectorPanel::renderTransformComponent(TransformComponent* tc) {
    if (!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    if (ImGui::BeginTable("PositionTable", 4, ImGuiTableFlags_SizingStretchSame))
    {
      ImGui::TableNextColumn();
      ImGui::Text("Position");

      ImGui::TableNextColumn();
      ImGui::Text("X:"); ImGui::SameLine();
      ImGui::DragFloat("##PX", &tc->position.x, 0.1f, -100000.0f, 100000.0f);

      ImGui::TableNextColumn();
      ImGui::Text("Y:"); ImGui::SameLine();
      ImGui::DragFloat("##PY", &tc->position.y, 0.1f, -100000.0f, 100000.0f);

      ImGui::TableNextColumn();
      ImGui::Text("Z:"); ImGui::SameLine();
      ImGui::DragFloat("##PZ", &tc->position.z, 0.1f, -100000.0f, 100000.0f);

      ImGui::EndTable();
    }

    if (ImGui::BeginTable("RotationTable", 4, ImGuiTableFlags_SizingStretchSame))
    {
      ImGui::TableNextColumn();
      ImGui::Text("Rotation");

      ImGui::TableNextColumn();
      ImGui::Text("X:"); ImGui::SameLine();
      ImGui::DragFloat("##RX", &tc->rotation.x, 0.1f, -360.0f, 360.0f);

      ImGui::TableNextColumn();
      ImGui::Text("Y:"); ImGui::SameLine();
      ImGui::DragFloat("##RY", &tc->rotation.y, 0.1f, -360.0f, 360.0f);

      ImGui::TableNextColumn();
      ImGui::Text("Z:"); ImGui::SameLine();
      ImGui::DragFloat("##RZ", &tc->rotation.z, 0.1f, -360.0f, 360.0f);

      ImGui::EndTable();
    }

    if (ImGui::BeginTable("ScaleTable", 4, ImGuiTableFlags_SizingStretchSame))
    {
      ImGui::TableNextColumn();
      ImGui::Text("Scale");

      ImGui::TableNextColumn();
      ImGui::Text("X:"); ImGui::SameLine();
      ImGui::DragFloat("##SX", &tc->scale.x, 0.1f, -360.0f, 360.0f);

      ImGui::TableNextColumn();
      ImGui::Text("Y:"); ImGui::SameLine();
      ImGui::DragFloat("##SY", &tc->scale.y, 0.1f, -360.0f, 360.0f);

      ImGui::TableNextColumn();
      ImGui::Text("Z:"); ImGui::SameLine();
      ImGui::DragFloat("##SZ", &tc->scale.z, 0.1f, -360.0f, 360.0f);

      ImGui::EndTable();
    }

  }

  void InspectorPanel::renderRenderComponent(RenderComponent* rc, World* world) {
    if (!ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    ImGui::Spacing();

    ImGui::TextDisabled("Mesh");
    std::string meshDrop = dropTarget("Mesh", "MESH_PATH", droppedMeshPath_);
    if (!meshDrop.empty()) {
        if (meshDrop == "CLEARED") {
            droppedMeshPath_ = "None";
            rc->mesh = nullptr;
        } else {
            droppedMeshPath_ = meshDrop;

            // TODO: Load mesh

        }
    }

    ImGui::Spacing();

    // TODO: Implement texture drop
    ImGui::TextDisabled("Texture");
    std::string matDrop = dropTarget("Texture", "TEXTURE_PATH", droppedTexturePath_);
    if (!matDrop.empty()) {
        if (matDrop == "CLEARED") {
            droppedTexturePath_ = "None";

            rc->material = nullptr;
        } else {
            droppedTexturePath_ = matDrop;
            // TODO: Update texture of the render component
        }
    }
  }

} // namespace mam
