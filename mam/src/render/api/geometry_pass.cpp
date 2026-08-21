  #include "render/api/geometry_pass.hpp"
  #include "render/api/graphics_device.hpp"
  #include "render/api/renderer.hpp"
  #include "render/api/mesh.hpp"
  #include "render/api/pipeline.hpp"
  #include "render/api/shader.hpp"
  #include "render/vulkan/vk_device.hpp"
  #include "matsys/material.hpp"
  #include "core/utils.hpp"

  namespace mam {

    void GeometryPass::init(u32 w, u32 h) {
      if (dynamic_cast<VKDevice*>(&gd_) != nullptr) return;

      FrameBufferSpec spec;
      spec.width        = w;
      spec.height       = h;
      spec.colorFormats = { Format::RGBA32F, Format::RGBA16F,
                                Format::RGBA8,   Format::RGBA8 };
      spec.hasDepth    = true;
      spec.depthFormat = Format::DEPTH24STENCIL8;
      gBuffer_ = gd_.createFrameBuffer(spec);

      {
        auto vs = gd_.createShader();
        auto fs = gd_.createShader();
        vs->loadSourceFromFile(Shader::Type::Vertex,
            "../assets/shaders/gbuffer_vert.glsl");
        fs->loadSourceFromFile(Shader::Type::Fragment,
            "../assets/shaders/gbuffer_frag.glsl");
        vs->compile(); fs->compile();
        pipeline_ = gd_.createPipeline();
        pipeline_->addShader(vs.get());
        pipeline_->addShader(fs.get());
        auto err = pipeline_->create();
        assert(!err.has_value() && "Failed to link GBuffer pipeline!");
      }

      {
        auto vs = gd_.createShader();
        auto fs = gd_.createShader();
        vs->loadSourceFromFile(Shader::Type::Vertex,
            "../assets/shaders/gbuffer_terrain_vert.glsl");
        fs->loadSourceFromFile(Shader::Type::Fragment,
            "../assets/shaders/gbuffer_terrain_frag.glsl");
        vs->compile(); fs->compile();
        terrainPipeline_ = gd_.createPipeline();
        terrainPipeline_->addShader(vs.get());
        terrainPipeline_->addShader(fs.get());
        auto err = terrainPipeline_->create();
        assert(!err.has_value() && "Failed to link GBuffer terrain pipeline!");
      }
    }

    void GeometryPass::execute(const RenderPassContext& ctx) {
      if (!gBuffer_) return;

      gBuffer_->bind();
      gd_.setClearColor(0.f, 0.f, 0.f, 1.f);
      gd_.clearBuffers();
      gd_.enableDepthTest();
      gd_.enableFaceCulling();

      static const char* kGenericSamplers[] = {
        "u_albedoMap", "u_normalMap", "u_roughnessMap", "u_metallicMap"
      };
      static const char* kGenericUseFlags[] = {
        "u_useAlbedoMap", "u_useNormalMap", "u_useRoughnessMap", "u_useMetallicMap"
      };
      static const char* kTerrainSamplers[] = {
        "u_texture0", "u_texture1", "u_texture2", "u_texture3"
      };
      static constexpr u32 kMaxTextureSlots = 4;

      Pipeline* activePipeline = nullptr;

      for (auto& cmd : ctx.queue->getRenderCommands()) {
        if (!cmd.mesh) continue;

        Pipeline* desired = cmd.useSplatting ? terrainPipeline_.get() : pipeline_.get();
        if (desired != activePipeline) {
          activePipeline = desired;
          activePipeline->use();
          activePipeline->setUniform("u_view",       glm::value_ptr(ctx.view));
          activePipeline->setUniform("u_projection", glm::value_ptr(ctx.projection));
        }

        activePipeline->setUniform("u_model", glm::value_ptr(cmd.transform));

        if (cmd.useSplatting) {
          for (u32 slot = 0; slot < static_cast<u32>(cmd.textures.size()) && slot < kMaxTextureSlots; ++slot) {
            if (!cmd.textures[slot]) continue;
            cmd.textures[slot]->bind(slot);
            activePipeline->setSamplerUniform(kTerrainSamplers[slot], static_cast<int>(slot));
          }
        } else {
          for (u32 i = 0; i < kMaxTextureSlots; ++i) {
            int value = 0;
            activePipeline->setUniform(kGenericUseFlags[i], &value);
          }
          for (u32 slot = 0; slot < static_cast<u32>(cmd.textures.size()) && slot < kMaxTextureSlots; ++slot) {
            if (!cmd.textures[slot]) continue;
            cmd.textures[slot]->bind(slot);
            activePipeline->setSamplerUniform(kGenericSamplers[slot], static_cast<int>(slot));
            int value = 1;
            activePipeline->setUniform(kGenericUseFlags[slot], &value);
          }
        }

        gd_.draw(*cmd.mesh);
      }

      gBuffer_->unbind();
    }

    void GeometryPass::resize(u32 w, u32 h) {
      if (!gBuffer_) return;

      gBuffer_->resize(w, h);
    }
  }