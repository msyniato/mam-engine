
#include "ui/gl_imgui_wrapper.hpp"
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "render/api/graphics_context.hpp"

#include "ecs/system.hpp"

namespace mam {
  
  void GLImGUIWrapper::initImplementation(GLFWwindow* window) {
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");   
  }

  void GLImGUIWrapper::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
  }
  
  void GLImGUIWrapper::endFrame() {
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  }
  
  void GLImGUIWrapper::shutdownImplementation() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
  }
  
} //namespace mam
