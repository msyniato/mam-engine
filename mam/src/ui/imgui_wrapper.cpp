

#include "ui/imgui_wrapper.hpp"

#include "ecs/world.hpp"

#include "render/api/frame_buffer.hpp"

#include <imgui.h>

#include "core/camera.hpp"

static bool showWorldOutliner = false;
static bool showFileExplorer = false;
static bool showInspector = false;
static bool showProfiler = false;
static bool showCameraSettings = false;
static bool showTerrainGen = false;

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
  
  void ImGUIWrapper::init(GLFWwindow* window, GraphicsContext* graphicsContext) {
    
    graphicsContext_ = graphicsContext;

    glfwGetWindowSize(window, &lastViewportSize_.x, &lastViewportSize_.y);
    
    initWrapper();
    initImplementation(window);
  }
  
  void ImGUIWrapper::initWrapper() {
    currentEntity_ = kInvalidEntity;
    isCameraMovable_ = false;
    
    // load window states
    std::ifstream file("../assets/interface/state.ini");
    file >> showInspector;
    file >> showWorldOutliner;
    file >> showFileExplorer;
    file >> showProfiler;
    file >> showCameraSettings;
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.IniFilename = "../assets/interface/layout.ini";
    
    ImFont* myFont = io.Fonts->AddFontFromFileTTF(
        "../assets/fonts/JetbrainsMonoRegular-RpvmM.ttf",
        11.0f                             
    );
    io.FontDefault = myFont;
    
    // set ImGUI style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowMinSize        = ImVec2( 160, 20 );
    style.FramePadding         = ImVec2( 4, 2 );
    style.ItemSpacing          = ImVec2( 6, 2 );
    style.ItemInnerSpacing     = ImVec2( 6, 4 );
    style.Alpha                = 0.95f;
    style.WindowRounding       = 4.0f;
    style.FrameRounding        = 2.0f;
    style.IndentSpacing        = 6.0f;
    style.ItemInnerSpacing     = ImVec2( 2, 4 );
    style.ColumnsMinSpacing    = 50.0f;
    style.GrabMinSize          = 14.0f;
    style.GrabRounding         = 16.0f;
    style.ScrollbarSize        = 12.0f;
    style.ScrollbarRounding    = 16.0f;
    style.FontScaleDpi         = 1.25f;
    style.FontScaleMain        = 1.25f;
    style.FontSizeBase         = 11.0f;
    
    style.Colors[ImGuiCol_Text]                  = ImVec4(0.86f, 0.93f, 0.89f, 0.78f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.86f, 0.93f, 0.89f, 0.28f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.31f, 0.31f, 1.00f, 0.00f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.20f, 0.22f, 0.27f, 0.75f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.20f, 0.22f, 0.27f, 0.47f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.09f, 0.15f, 0.16f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.92f, 0.18f, 0.29f, 0.76f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_Separator]             = ImVec4(0.14f, 0.16f, 0.19f, 1.00f);
    style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
    style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.92f, 0.18f, 0.29f, 0.43f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.20f, 0.22f, 0.27f, 0.9f);
    //style.Colors[ImGuiCol_ModalWindowDarkening]  = ImVec4(0.20f, 0.22f, 0.27f, 0.73f);
  }
  
  void ImGUIWrapper::shutdown() {
    shutdownImplementation();
    shutdownWrapper();
  }

  void ImGUIWrapper::shutdownWrapper() {
    ImGui::DestroyContext();
    
    // save window states
    std::ofstream file("../assets/interface/state.ini");
    file << showInspector << "\n";
    file << showWorldOutliner << "\n";
    file << showFileExplorer << "\n";
    file << showProfiler << "\n";
    file << showCameraSettings << "\n";
  }
  
  void ImGUIWrapper::render(Context& context) {
    
    renderMainWindow();
    renderViewport(context);
    
    if (showWorldOutliner) renderWorldOutliner(context);
    if (showFileExplorer)  renderFileExplorer();
    if (showInspector)     renderInspector(context);
    if (showProfiler)      renderProfiler(context);
    if (showCameraSettings) renderCameraSettings(context);
    if (showTerrainGen) terrainGenPanel_.render(&showTerrainGen);
    if (extraImGuiCb_) extraImGuiCb_(context);
  }

  void ImGUIWrapper::renderDummyFrame() {
    ImGui::Begin("Warmup");
    
    for (int i = 0; i < 2000; i++) {
      ImGui::Text("Warming up %d", i);
    }

    ImGui::End();
    
  }

  void ImGUIWrapper::renderMainWindow() {
    ImGuiWindowFlags host_flags = ImGuiWindowFlags_MenuBar;
    host_flags |= ImGuiWindowFlags_NoTitleBar;
    host_flags |= ImGuiWindowFlags_NoCollapse;
    host_flags |= ImGuiWindowFlags_NoResize;
    host_flags |= ImGuiWindowFlags_NoMove;
    host_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
    host_flags |= ImGuiWindowFlags_NoNavFocus;
    host_flags |= ImGuiWindowFlags_NoBackground;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("DockSpace Host", nullptr, host_flags);
    
    // Top bar
    if (ImGui::BeginMenuBar())
    {
      if (ImGui::BeginMenu("File"))
      {
        
        ImGui::EndMenu();
      }
      
      if (ImGui::BeginMenu("Window"))
      {
        ImGui::MenuItem("Inspector", nullptr, &showInspector);
        ImGui::MenuItem("World Outliner", nullptr, &showWorldOutliner);
        ImGui::MenuItem("File Explorer", nullptr, &showFileExplorer);
        ImGui::MenuItem("Profiler", nullptr, &showProfiler);
        ImGui::MenuItem("Camera Settings", nullptr, &showCameraSettings);
        ImGui::MenuItem("World Generation", nullptr, &showTerrainGen);

        ImGui::EndMenu();
      }
      ImGui::EndMenuBar();
    }

    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    
    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags); // ImGuiWindowFlags_None
    ImGui::End();
    
    ImGui::PopStyleVar(3);
  }

  void ImGUIWrapper::renderViewport(Context& context) {
    ImGuiWindowFlags viewport_flags = ImGuiWindowFlags_NoCollapse;
    viewport_flags |= ImGuiWindowFlags_NoMove; 
    
    ImGui::Begin("Scene", nullptr, viewport_flags); 
    
    
    ImVec2 size = ImGui::GetContentRegionAvail();
    static ImVec2 lastSize = size; 

    auto fb = context.Get<FrameBuffer>();
    if (fb && (size.x != lastSize.x || size.y != lastSize.y)) {
      if (size.x > 0 && size.y > 0) {
        fb->resize(static_cast<u32>(size.x), static_cast<u32>(size.y));
        lastSize = size;
      }
    }

    isCameraMovable_ = ImGui::IsWindowHovered();
    lastViewportSize_ = { size.x, size.y };

    if (!fb) {
      ImGui::Text("Framebuffer not ready.");
      ImGui::End();
      return;
    }

    if (fb->getColorAttachments().empty()) {
      ImGui::Text("Framebuffer has no color attachment.");
      ImGui::End();
      return;
    }

    if (graphicsContext_ &&
      graphicsContext_->getRenderAPI() == GraphicsContext::RenderAPI::Vulkan) {
      ImGui::Text("Vulkan viewport texture pending ImGui descriptor.");
    }
    else {
      ImGui::Image(
        (ImTextureID)(u64)fb->getColorAttachments()[0]->id(),
        size,
        ImVec2(0, 1),
        ImVec2(1, 0)
      );
      ImVec2 mn = ImGui::GetItemRectMin();          
      lastViewportMin_ = { mn.x, mn.y };
    }
    
    ImGui::End();
  }

  void ImGUIWrapper::renderWorldOutliner(Context& context) {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoResize;
    
    auto world = context.Get<World>();
    
    ImGui::Begin("World Outliner", &showWorldOutliner, window_flags);
      
    for (auto& arch : world->getArchetypes()) {
        
      auto& entities = arch->entities();
      for (size_t i = 0; i < arch->getEntitiesNum(); i++) {
          
        auto handle = world->getHandle(entities[i]);
        
        bool isSelected = (inspector_.getSelectedEntity().id() == handle.entity().id());
        
        std::string label = handle.name() + "##" + std::to_string(handle.entity().id()); 
        if (ImGui::Selectable(label.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
        {
          inspector_.setSelectedEntity(entities[i]);
          showInspector = true;
        }
        
        ImGui::Spacing();
      }
    }
    ImGui::End();
  }

  void ImGUIWrapper::renderFileExplorer() {
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoResize;
    
    ImGui::Begin("File Explorer", &showFileExplorer, window_flags);
      
    fileExplorer_.render();
      
    ImGui::End();
    
  }

  void ImGUIWrapper::renderInspector(Context& context) {
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoResize;
    
    auto world = context.Get<World>();
    
    ImGui::Begin("Inspector", &showInspector, window_flags);
    inspector_.render(world);
    ImGui::End();
    
  }

  void ImGUIWrapper::renderProfiler(Context& context) {

    auto world = context.Get<World>();
		auto camera = context.Get<Camera>();

    ImGui::Begin("Profiler",  &showProfiler, ImGuiWindowFlags_NoCollapse);
    ImGui::Text("FPS: %.2f ", 1.0f / (world->lastDeltaTime != 0.0f ? world->lastDeltaTime : 1.0f));
    ImGui::Text("Camera position: %.2f, %.2f, %.2f", camera->position().x, camera->position().y, camera->position().z);
    ImGui::End();
  }

  void ImGUIWrapper::renderCameraSettings(Context& context) {
    
    auto camera = context.Get<Camera>();
    
    ImGui::Begin("Camera Settings",  &showCameraSettings, ImGuiWindowFlags_NoCollapse);
    
    float speed = camera->speed();
    ImGui::Text("Camera speed");
    ImGui::DragFloat("##speed", &speed, 10.0f, 10.0f, 1000.0f, "%.1f");
    camera->setSpeed(speed);
    
    ImGui::Spacing();
    
    float sensitivity = camera->sensitivity();
    ImGui::Text("Camera sensitivity");
    ImGui::DragFloat("##sens", &sensitivity, 0.1f, 0.1f, 5,"%.2f");
    camera->setSensitivity(sensitivity);
    
    ImGui::Spacing();
    
    float zNear = camera->zNear();
    ImGui::Text("Near clip plane");
    ImGui::DragFloat("##znear", &zNear, 0.1f, 0.1f,10.0f, "%.1f");
    camera->setZNear(zNear);
    
    ImGui::Spacing();
    
    float zFar = camera->zFar();
    ImGui::Text("Near clip plane");
    ImGui::DragFloat("##zfar", &zFar, 100.0f, 100.0f, 100000.0f, "%.1f");
    camera->setZFar(zFar);
    
    ImGui::End();
  }
} //namespace mam 