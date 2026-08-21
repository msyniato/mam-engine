#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h> 
#include <mam/engine.hpp>

int main(int argc, char** argv)
{
  mam::GraphicsContext::Desc desc = {
    1000,
    1200,
    mam::GraphicsContext::RenderAPI::OpenGL
  };

  mam::Engine engine(desc);
  engine.init();

  auto window = engine.getWindow();
  auto context = std::make_unique<mam::Context>();

  mam::AudioManager audio;
  audio.CreateContext();

  #pragma region LOAD_SOURCES

  // --- WAV ---
  mam::SourceData musicSrc{};
  musicSrc.pitch = 1.0f;
  musicSrc.gain = 1.0f;
  musicSrc.position = glm::vec3{ 0.0f, 0.0f, 0.0f };
  musicSrc.velocity = glm::vec3{ 0.0f, 0.0f, 0.0f };
  musicSrc.loop = true;
  musicSrc.layer = mam::AudioLayer::MUSIC;  
  audio.CreateSource(musicSrc);

  mam::SourceData UISrc{};
  UISrc.pitch = 1.0f;
  UISrc.gain = 1.0f;
  UISrc.position = glm::vec3{ 0.0f, 0.0f, 0.0f };
  UISrc.velocity = glm::vec3{ 0.0f, 0.0f, 0.0f };
  UISrc.loop = true;
  UISrc.layer = mam::AudioLayer::UI;
  audio.CreateSource(UISrc);

  mam::SourceData sfxSrc{};
  sfxSrc.pitch = 1.0f;
  sfxSrc.gain = 1.0f;
  sfxSrc.position = glm::vec3{ 0.0f, 0.0f, 0.0f };
  sfxSrc.velocity = glm::vec3{ 0.0f, 0.0f, 0.0f };
  sfxSrc.loop = true;
  sfxSrc.layer = mam::AudioLayer::SFX;       
  audio.CreateSource(sfxSrc);

  mam::SourceData ambientSrc{};
  ambientSrc.pitch = 1.0f;
  ambientSrc.gain = 1.0f;
  ambientSrc.position = glm::vec3{ 0.0f, 0.0f, 0.0f };
  ambientSrc.velocity = glm::vec3{ 0.0f, 0.0f, 0.0f };
  ambientSrc.loop = true;
  ambientSrc.layer = mam::AudioLayer::AMBIENT;
  audio.CreateSource(ambientSrc);
  
  mam::SourceData crossfade{};
  crossfade.pitch = 1.0f;
  crossfade.gain = 1.0f;
  crossfade.position = glm::vec3{ 0.0f, 0.0f, 0.0f };
  crossfade.velocity = glm::vec3{ 0.0f, 0.0f, 0.0f };
  crossfade.loop = true;
  crossfade.layer = mam::AudioLayer::AMBIENT;
  audio.CreateSource(crossfade);

  bool ret;
  ret = audio.LoadOGGFile("../assets/audio/drums.ogg", 0);
  ret = audio.LoadOGGFile("../assets/audio/guitar.ogg", 1);
  ret = audio.LoadOGGFile("../assets/audio/song.ogg", 2);
  ret = audio.LoadOGGFile("../assets/audio/rhythm.ogg", 3);
  ret = audio.LoadWavFile("../assets/audio/bossanova.wav", 4);

	audio.PlaySource(0);
	audio.PlaySource(1);
	audio.PlaySource(2);
  audio.PlaySource(3);
  audio.PlaySource(4);
  #pragma endregion

  #pragma region VARIABLES

  bool isPlaying = false;

  float masterGain = audio.GetMasterGain();

  float crossFade = 0.0f;

  float musicLayerGain = audio.GetLayerGain(mam::AudioLayer::MUSIC);
  float sfxLayerGain = audio.GetLayerGain(mam::AudioLayer::SFX);
  float ambientLayerGain = audio.GetLayerGain(mam::AudioLayer::AMBIENT);
  float UILayerGain = audio.GetLayerGain(mam::AudioLayer::UI);

  bool mute0 = false;
  bool solo0 = false;
  float src0BaseGain = 1.0f;
  float src0Pitch = 1.0f;
  float src0PosX = 0.0f;
  float src0VelX = 0.0f;

  bool mute1 = false;
  bool solo1 = false;
  float src1BaseGain = 1.0f;
  float src1Pitch = 1.0f;
  float src1PosX = 0.0f;
  float src1VelX = 0.0f;

  bool mute2 = false;
  bool solo2 = false;
  float src2BaseGain = 1.0f;
  float src2Pitch = 1.0f;
  float src2PosX = 0.0f;
  float src2VelX = 0.0f;

  bool mute3 = false;
  bool solo3 = false;
  float src3BaseGain = 1.0f;
  float src3Pitch = 1.0f;
  float src3PosX = 0.0f;
  float src3VelX = 0.0f;

  bool mute4 = false;
  float src4BaseGain = 1.0f;
  float src4Pitch = 1.0f;

  static float wave0[256];
  static float wave1[256];
  static float wave2[256];
  static float wave3[256];
  for (int i = 0; i < 256; ++i) {
    float t = static_cast<float>(i);

    wave0[i] = sinf(t * 0.10f) + 0.3f * sinf(t * 0.25f);
    wave1[i] = 0.8f * sinf(t * 0.07f) + 0.4f * sinf(t * 0.19f + 1.0f);
    wave2[i] = 0.6f * sinf(t * 0.15f)
      + 0.3f * sinf(t * 0.30f)
      + 0.1f * sinf(t * 0.60f);

    wave3[i] = 0.4f * sinf(t * 0.05f)
      + 0.4f * sinf(t * 0.09f + 2.0f)
      + 0.2f * sinf(t * 0.02f);
  }

#pragma endregion

  while (!window->isClosed())
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

		ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::Begin("Audio Control");

    if (ImGui::Button("Play / Pause")) {
      if (isPlaying) {
        for (int i = 0; i < audio.GetSourceCount(); ++i)
          audio.PauseSource(i);
        isPlaying = false;
      }
      else {
        for (int i = 0; i < audio.GetSourceCount(); ++i)
          audio.PlaySource(i);
        isPlaying = true;
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
      for (int i = 0; i < audio.GetSourceCount(); ++i)
        audio.StopSource(i);
      isPlaying = false;
    }

    ImGui::Separator();

    ImGui::Text("Crossfade");
    ImGui::SliderFloat("##Crossfade", &crossFade, 0.0f, 1.0f);

    ImGui::Separator();

    ImGui::Text("Master");
    if (ImGui::SliderFloat("##MasterGain", &masterGain, 0.0f, 1.0f)) {
      audio.SetMasterGain(masterGain);
    }

    ImGui::End(); 

		ImGui::SetNextWindowPos(ImVec2(10, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("Source 1");

    ImGui::Checkbox("Mute##1", &mute0);
    ImGui::SameLine();
    ImGui::Checkbox("Solo##1", &solo0);
    ImGui::Separator();

    ImGui::Text("Pitch");
    if (ImGui::SliderFloat("##Pitch1", &src0Pitch, 0.5f, 2.0f)) {
      audio.SetSourcePitch(0, src0Pitch);
    }

    ImGui::Text("Gain");
    ImGui::SliderFloat("##Gain1", &src0BaseGain, 0.0f, 1.0f);

    ImGui::Separator();
    ImGui::Text("Wave0");
    ImGui::PlotLines("##Wave0", wave0, IM_ARRAYSIZE(wave0),
      0, nullptr, -2.0f, 2.0f, ImVec2(0, 60));

    ImGui::End(); 

		ImGui::SetNextWindowPos(ImVec2(250, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("Source 2");

    ImGui::Checkbox("Mute##2", &mute1);
    ImGui::SameLine();
    ImGui::Checkbox("Solo##2", &solo1);

    ImGui::Separator();

    ImGui::Text("Pitch");
    if (ImGui::SliderFloat("##Pitch2", &src1Pitch, 0.5f, 2.0f)) {
      audio.SetSourcePitch(1, src1Pitch);
    }

    ImGui::Text("Gain");
    ImGui::SliderFloat("##Gain2", &src1BaseGain, 0.0f, 1.0f);

    ImGui::Separator();
    ImGui::Text("Wave1");
    ImGui::PlotLines("##Wave1", wave1, IM_ARRAYSIZE(wave1),
      0, nullptr, -2.0f, 2.0f, ImVec2(0, 60));

    ImGui::End(); 

		ImGui::SetNextWindowPos(ImVec2(490, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("Source 3");

    ImGui::Checkbox("Mute##3", &mute2);
    ImGui::SameLine();
    ImGui::Checkbox("Solo##3", &solo2);

    ImGui::Separator();

    ImGui::Text("Pitch");
    if (ImGui::SliderFloat("##Pitch3", &src2Pitch, 0.5f, 2.0f)) {
      audio.SetSourcePitch(2, src2Pitch);
    }

    ImGui::Text("Gain");
    ImGui::SliderFloat("##Gain3", &src2BaseGain, 0.0f, 1.0f);

    ImGui::Separator();
    ImGui::Text("Wave2 / Drum");
    ImGui::PlotLines("##Wave2", wave2, IM_ARRAYSIZE(wave2),
      0, nullptr, -2.0f, 2.0f, ImVec2(0, 60));

    ImGui::End(); 

		ImGui::SetNextWindowPos(ImVec2(730, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("Source 4");

    ImGui::Checkbox("Mute##4", &mute3);
    ImGui::SameLine();
    ImGui::Checkbox("Solo##4", &solo3);

    ImGui::Separator();

    ImGui::Text("Pitch");
    if (ImGui::SliderFloat("##Pitch4", &src3Pitch, 0.5f, 2.0f)) {
      audio.SetSourcePitch(3, src3Pitch);
    }

    ImGui::Text("Gain");
    ImGui::SliderFloat("##Gain4", &src3BaseGain, 0.0f, 1.0f);

    ImGui::Separator();
    ImGui::Text("Wave3 / Ambient");
    ImGui::PlotLines("##Wave3", wave3, IM_ARRAYSIZE(wave3),
      0, nullptr, -2.0f, 2.0f, ImVec2(0, 60));

    ImGui::End();

    {
      float base0 = src0BaseGain;
      float base1 = src1BaseGain;
      float base2 = src2BaseGain;
      float base3 = src3BaseGain;
      float base4 = src4BaseGain;

      if (mute0) base0 = 0.0f;
      if (mute1) base1 = 0.0f;
      if (mute2) base2 = 0.0f;
      if (mute3) base3 = 0.0f;
      if (mute4) base4 = 0.0f;

      float effGain0 = base0 * (1.0f - crossFade);
      float effGain1 = base1 * (1.0f - crossFade);
      float effGain2 = base2 * (1.0f - crossFade);
      float effGain3 = base3 * (1.0f - crossFade);
      float effGain4 = base4 * crossFade;

      audio.SetSourceGain(0, effGain0);
      audio.SetSourceGain(1, effGain1);
      audio.SetSourceGain(2, effGain2);
      audio.SetSourceGain(3, effGain3);
      audio.SetSourceGain(4, effGain4);
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    engine.update(*context);
  }

  engine.cleanup();

  return 0;
}