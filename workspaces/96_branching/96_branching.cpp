
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imnodes.h>
#include <mam/engine.hpp>

struct AudioNode {
  int id;
  std::string name;
  int audioSourceIndex;
  int inputAttr;
  int outputAttr;
};

struct AudioLink {
  int id;
  int startAttr;
  int endAttr;
  bool yellow = false;
};

struct AudioGraph {
  std::unordered_map<int, AudioNode> nodes;
  std::vector<AudioLink> links;
  int nextNodeId = 1;
  int nextLinkId = 1;
  int activeNodeId = -1;
  int playingSrc = -1;
  mam::AudioManager* audio = nullptr;

  int createNode(const std::string& name, int srcIdx) {
    int id = nextNodeId++;
    nodes[id] = { id, name, srcIdx, 200 + id, 100 + id };
    return id;
  }

  void pushLink(int start, int end, bool yellow = false) {
    if (nodes.count(start) && nodes.count(end)) {
      links.push_back({ nextLinkId++, nodes[start].outputAttr, nodes[end].inputAttr, yellow });
    }
  }

  void connectLayer(const std::vector<int>& layer) {
    for (size_t i = 0; i < layer.size(); ++i)
      pushLink(layer[i], layer[(i + 1) % layer.size()]);
  }

  void stopAll() {
    if (!audio) return;
    for (auto& [id, node] : nodes) {
      if (audio->IsSourcePlaying(node.audioSourceIndex)) {
        audio->StopSource(node.audioSourceIndex);
      }
    }
    playingSrc = -1;
    activeNodeId = -1;
  }

  void playNode(int nodeId) {
    if (!audio) return;
    if (!nodes.count(nodeId)) return;

    int src = nodes[nodeId].audioSourceIndex;

    if (playingSrc == src && audio->IsSourcePlaying(src)) {
      activeNodeId = nodeId;
      return;
    }

    for (auto& [id, node] : nodes) {
      if (node.audioSourceIndex != src) {
        if (audio->IsSourcePlaying(node.audioSourceIndex))
          audio->StopSource(node.audioSourceIndex);
      }
    }

    if (audio->IsSourcePlaying(src))
      audio->StopSource(src);

    audio->PlaySource(src);
    playingSrc = src;
    activeNodeId = nodeId;
  }
};

int main() {
  mam::GraphicsContext::Desc desc = {
  1000,
  1200,
  mam::GraphicsContext::RenderAPI::OpenGL
  };

  mam::Engine engine(desc);
  engine.init();

  auto window = engine.getWindow();
  auto context = std::make_unique<mam::Context>();

  const char* glsl_version = "#version 130";

  ImGuiIO& io = ImGui::GetIO(); (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.DisplaySize = ImGui::GetMainViewport()->Size;

  ImNodes::CreateContext();
  ImGui::StyleColorsDark();
  ImNodes::StyleColorsDark();


#pragma region AUDIO_MANAGER
  mam::AudioManager audio;
  audio.CreateContext();

  for (int i = 0; i < 8; ++i) {
    mam::SourceData s{};
    s.pitch = 1.0f; s.gain = 1.0f;
    s.position = glm::vec3(0.0f);
    s.velocity = glm::vec3(0.0f);
    s.loop = false;
    s.layer = mam::AudioLayer::MUSIC;
    audio.CreateSource(s);
  }

  audio.LoadWavFile("../assets/audio/A01.wav", 0); // A01
  audio.LoadWavFile("../assets/audio/A02.wav", 1); // A02
  audio.LoadWavFile("../assets/audio/A03.wav", 2); // A03
  audio.LoadWavFile("../assets/audio/EAB.wav", 3); // E_AB
  audio.LoadWavFile("../assets/audio/B01.wav", 4); // B01
  audio.LoadWavFile("../assets/audio/B02.wav", 5); // B02
  audio.LoadWavFile("../assets/audio/B03.wav", 6); // B03
  audio.LoadWavFile("../assets/audio/EBA.wav", 7); // E_BA

  float masterGain = audio.GetMasterGain();

#pragma endregion

#pragma region AUDIOGRAPH
  AudioGraph graph;
  graph.audio = &audio;

  std::vector<int> layerA, layerB;
  int nodeE_AB, nodeE_BA;

  layerA = { graph.createNode("A01", 0), graph.createNode("A02", 1), graph.createNode("A03", 2) };
  nodeE_AB = graph.createNode("E_AB", 3);
  layerB = { graph.createNode("B01", 4), graph.createNode("B02", 5), graph.createNode("B03", 6) };
  nodeE_BA = graph.createNode("E_BA", 7);

  graph.connectLayer(layerA);
  graph.connectLayer(layerB);
  for (int n : layerA) graph.pushLink(n, nodeE_AB, true);
  for (int n : layerB) graph.pushLink(n, nodeE_BA, true);

#pragma endregion

  enum class Phase { LOOP_A, EVENT_A_TO_B, LOOP_B, EVENT_B_TO_A };
  Phase phase = Phase::LOOP_A;
  Phase phaseNext = phase;
  int activeIndex = 0;
  bool changeRequested = false;
  bool waitingEvent = false;
  int eventSrc = -1;

  graph.playNode(layerA[activeIndex]);

  while (!window->isClosed()) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

#pragma region IMGUI
    ImGui::Begin("Audio Manager");
    if (ImGui::CollapsingHeader("Control Panel", ImGuiTreeNodeFlags_DefaultOpen)) {
      if (ImGui::Button("Change Event")) changeRequested = true;

      ImGui::SameLine();
      if (ImGui::Button("Stop All")) graph.stopAll();

      ImGui::Separator();
      ImGui::Text("Master");
      if (ImGui::SliderFloat("##MasterGain", &masterGain, 0.0f, 1.0f))
        audio.SetMasterGain(masterGain);
    }

    if (ImGui::CollapsingHeader("Nodes", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImNodes::BeginNodeEditor();
      for (auto& [id, node] : graph.nodes) {
        bool isActive = (graph.activeNodeId == node.id);
        if (isActive) {
          ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(200, 40, 40, 255));
          ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, IM_COL32(230, 60, 60, 255));
          ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, IM_COL32(255, 80, 80, 255));
        }
        ImNodes::BeginNode(node.id);
        ImNodes::BeginNodeTitleBar();
        ImGui::Text("%s", node.name.c_str());
        ImNodes::EndNodeTitleBar();
        ImNodes::BeginInputAttribute(node.inputAttr);
        ImGui::Text("E");
        ImNodes::EndInputAttribute();
        ImNodes::BeginOutputAttribute(node.outputAttr);
        ImGui::Text("C");
        ImNodes::EndOutputAttribute();
        ImNodes::EndNode();
        if (isActive) {
          ImNodes::PopColorStyle();
          ImNodes::PopColorStyle();
          ImNodes::PopColorStyle();
        }
      }
      for (auto& link : graph.links) {
        ImNodes::PushColorStyle(ImNodesCol_Link, link.yellow ? IM_COL32(240, 200, 40, 220) : IM_COL32(80, 160, 255, 220));
        ImNodes::PushColorStyle(ImNodesCol_LinkHovered, link.yellow ? IM_COL32(255, 230, 120, 255) : IM_COL32(120, 200, 255, 255));
        ImNodes::Link(link.id, link.startAttr, link.endAttr);
        ImNodes::PopColorStyle(); ImNodes::PopColorStyle();
      }
      ImNodes::EndNodeEditor();
    }
    ImGui::End();

#pragma endregion

#pragma region AUDIO_LOGIC
    if (waitingEvent) {
      if (!audio.IsSourcePlaying(eventSrc)) {
        waitingEvent = false;
        graph.playingSrc = -1;
        phase = phaseNext;
        if (phase == Phase::LOOP_A) {
          activeIndex = 0;
          graph.playNode(layerA[activeIndex]);
        } else if (phase == Phase::LOOP_B) {
          activeIndex = 0;
          graph.playNode(layerB[activeIndex]);
        }
      }
    } else {
      int nodeId = (phase == Phase::LOOP_A) ? layerA[activeIndex] :
        (phase == Phase::LOOP_B) ? layerB[activeIndex] : -1;

      if (nodeId != -1 && !audio.IsSourcePlaying(graph.nodes[nodeId].audioSourceIndex)) {
        if (changeRequested) {
          changeRequested = false;
          graph.stopAll();

          if (phase == Phase::LOOP_A) {
            phaseNext = Phase::LOOP_B;
            phase = Phase::EVENT_A_TO_B;
            eventSrc = graph.nodes[nodeE_AB].audioSourceIndex;
            waitingEvent = true;
            graph.playNode(nodeE_AB);
          } else if (phase == Phase::LOOP_B) {
            phaseNext = Phase::LOOP_A;
            phase = Phase::EVENT_B_TO_A;
            eventSrc = graph.nodes[nodeE_BA].audioSourceIndex;
            waitingEvent = true;
            graph.playNode(nodeE_BA);
          }
        } else {
          if (phase == Phase::LOOP_A) {
            activeIndex = (activeIndex + 1) % layerA.size();
            graph.playNode(layerA[activeIndex]);
          } else if (phase == Phase::LOOP_B) {
            activeIndex = (activeIndex + 1) % layerB.size();
            graph.playNode(layerB[activeIndex]);
          }
        }
      }
    }

#pragma endregion

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    engine.update(*context);
  }

  //ImGui_ImplOpenGL3_Shutdown();
  //ImGui_ImplGlfw_Shutdown();
  ImNodes::DestroyContext();
  //ImGui::DestroyContext();
  engine.cleanup();

  return 0;
}
