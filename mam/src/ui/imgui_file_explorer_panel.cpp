#include "ui/imgui_wrapper.hpp"

#include <imgui.h>

namespace mam {

  static ImVec4 getFileColor(const std::filesystem::path& path) {
    std::string ext = path.extension().string();
    if (ext == ".glsl" || ext == ".vert" || ext == ".frag") return ImVec4(0.4f, 0.8f, 1.0f, 1.0f);
    if (ext == ".png"  || ext == ".jpg"  || ext == ".tga")  return ImVec4(0.8f, 0.6f, 1.0f, 1.0f);
    if (ext == ".obj"  || ext == ".fbx"  || ext == ".gltf") return ImVec4(1.0f, 0.7f, 0.3f, 1.0f);
    if (ext == ".ini"  || ext == ".json" || ext == ".yaml") return ImVec4(0.6f, 1.0f, 0.6f, 1.0f);
    return ImVec4(0.86f, 0.93f, 0.89f, 0.78f);
  }

  static const char* getFileIcon(const std::filesystem::path& path) {
    if (std::filesystem::is_directory(path)) return "[D]";
    std::string ext = path.extension().string();
    if (ext == ".glsl" || ext == ".vert" || ext == ".frag") return "[S]";
    if (ext == ".png"  || ext == ".jpg"  || ext == ".tga")  return "[T]";
    if (ext == ".obj"  || ext == ".fbx"  || ext == ".gltf") return "[M]";
    return "[F]";
  }

  void FileExplorerPanel::render() {

    ImGui::BeginChild("##dir_tree", ImVec2(160, 0), true);
    renderDirectoryTree(rootPath_);
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##content", ImVec2(0, 0), true);
    renderContentBrowser();
    ImGui::EndChild();
  }

  void FileExplorerPanel::renderDirectoryTree(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) return;

    for (auto& entry : std::filesystem::directory_iterator(path)) {
      if (!std::filesystem::is_directory(entry)) continue;

      ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
                               | ImGuiTreeNodeFlags_SpanAvailWidth;

      if (entry.path() == currentPath_)
        flags |= ImGuiTreeNodeFlags_Selected;

      bool hasSubDirs = false;
      for (auto& sub : std::filesystem::directory_iterator(entry))
        if (std::filesystem::is_directory(sub)) { hasSubDirs = true; break; }

      if (!hasSubDirs)
        flags |= ImGuiTreeNodeFlags_Leaf;

      bool open = ImGui::TreeNodeEx(entry.path().filename().string().c_str(), flags);

      if (ImGui::IsItemClicked())
        currentPath_ = entry.path();

      if (open) {
        renderDirectoryTree(entry.path());
        ImGui::TreePop();
      }
    }
  }

  void FileExplorerPanel::renderContentBrowser() {

    ImGui::TextDisabled("%s", currentPath_.string().c_str());
    ImGui::Separator();

    if (currentPath_ != rootPath_) {
      if (ImGui::SmallButton(".."))
        currentPath_ = currentPath_.parent_path();
      ImGui::Separator();
    }

    if (!std::filesystem::exists(currentPath_)) return;

    std::vector<std::filesystem::path> dirs, files;
    for (auto& entry : std::filesystem::directory_iterator(currentPath_)) {
      if (std::filesystem::is_directory(entry)) dirs.push_back(entry.path());
      else                                       files.push_back(entry.path());
    }
    std::sort(dirs.begin(),  dirs.end());
    std::sort(files.begin(), files.end());

    auto renderEntry = [&](const std::filesystem::path& p) {
      bool isSelected = (p.string() == selectedFile_);

      ImGui::PushStyleColor(ImGuiCol_Text, getFileColor(p));

      ImGuiSelectableFlags selFlags = ImGuiSelectableFlags_SpanAllColumns;
      if (ImGui::Selectable(
          (std::string(getFileIcon(p)) + "  " + p.filename().string()).c_str(),
          isSelected,
          selFlags
      )) {
        handleFileClick(p);
      }

      if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        if (std::filesystem::is_directory(p))
          currentPath_ = p;

      if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {

        std::string ext = p.extension().string();
        const char* payloadType = "FILE_PATH";

        if (ext == ".png" || ext == ".jpg" || ext == ".tga")
          payloadType = "TEXTURE_PATH";
        else if (ext == ".obj" || ext == ".fbx" || ext == ".gltf")
          payloadType = "MESH_PATH";
        else if (ext == ".lua" || ext == ".py")
          payloadType = "SCRIPT_PATH";
        else if (ext == ".glsl" || ext == ".vert" || ext == ".frag")
          payloadType = "SHADER_PATH";

        std::string pathStr = p.string();
        ImGui::SetDragDropPayload(payloadType, pathStr.c_str(), pathStr.size() + 1);

        ImGui::Text("%s %s", getFileIcon(p), p.filename().string().c_str());

        ImGui::EndDragDropSource();
      }

      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", p.string().c_str());

      ImGui::PopStyleColor();
    };

    for (auto& d : dirs)  renderEntry(d);
    for (auto& f : files) renderEntry(f);
  }

  void FileExplorerPanel::handleFileClick(const std::filesystem::path& path) {
    if (std::filesystem::is_directory(path)) {
      selectedFile_ = path.string();
      return;
    }
    selectedFile_ = path.string();

    // TODO: hook into asset system here
    // e.g. if .glsl -> open in shader editor
    //      if .obj  -> load into scene
  }

} // namespace mam
