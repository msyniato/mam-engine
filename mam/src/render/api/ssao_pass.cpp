#include "render/api/ssao_pass.hpp"

#include "render/api/graphics_device.hpp"
#include "render/api/pipeline.hpp"
#include "render/api/shader.hpp"
#include "render/api/vertex_array.hpp"
#include "render/api/buffer.hpp"
#include "render/opengl/gl_texture.hpp"  
#include "render/vulkan/vk_device.hpp"  
#include <random>

namespace mam {

SSAOPass::~SSAOPass()
{
    if (queryId_) {
        glDeleteQueries(1, &queryId_);
        queryId_ = 0;
    }
}

void SSAOPass::init(u32 w, u32 h)
{
  if (dynamic_cast<VKDevice*>(&gd_) != nullptr) return;
    width_  = w;
    height_ = h;

    FrameBufferSpec spec;
    spec.width        = w;
    spec.height       = h;
    spec.colorFormats = { Format::RGBA8 };
    spec.hasDepth     = false;
    rawOutput_  = gd_.createFrameBuffer(spec);
    blurOutput_ = gd_.createFrameBuffer(spec);

    {
        auto vs = gd_.createShader();
        auto fs = gd_.createShader();

        vs->loadSourceFromFile(Shader::Type::Vertex,
            "../assets/shaders/deferred_light_vert.glsl");
        fs->loadSourceFromFile(Shader::Type::Fragment,
            "../assets/shaders/ssao_frag.glsl");
        vs->compile();
        fs->compile();

        ssaoPipeline_ = gd_.createPipeline();
        ssaoPipeline_->addShader(vs.get());
        ssaoPipeline_->addShader(fs.get());
        auto err = ssaoPipeline_->create();
        assert(!err.has_value() && "Failed to link SSAO pipeline!");
    }

    {
        auto vs = gd_.createShader();
        auto fs = gd_.createShader();
        vs->loadSourceFromFile(Shader::Type::Vertex,
            "../assets/shaders/deferred_light_vert.glsl");
        fs->loadSourceFromFile(Shader::Type::Fragment,
            "../assets/shaders/ssao_blur_frag.glsl");
        vs->compile();
        fs->compile();

        blurPipeline_ = gd_.createPipeline();
        blurPipeline_->addShader(vs.get());
        blurPipeline_->addShader(fs.get());
        auto err = blurPipeline_->create();
        assert(!err.has_value() && "Failed to link SSAO Blur pipeline!");
    }

    generateSamples();
    generateNoiseTexture();
    initQuad();

    glGenQueries(1, &queryId_);
}

void SSAOPass::execute(const RenderPassContext& ctx)
{
    if (!gBuffer_) return;

    auto& gbAttachments = gBuffer_->getColorAttachments();
    if (gbAttachments.size() < 2) return;

    {
        GLuint available = 0;
        glGetQueryObjectuiv(queryId_, GL_QUERY_RESULT_AVAILABLE, &available);
        if (available) {
            GLuint64 elapsed = 0;
            glGetQueryObjectui64v(queryId_, GL_QUERY_RESULT, &elapsed);
            lastPassMs_ = static_cast<float>(elapsed) * 1e-6f; 
        }
    }

    glBeginQuery(GL_TIME_ELAPSED, queryId_);

    gd_.disableDepthTest();

    const glm::vec2 noiseScale(
        static_cast<float>(width_)  / 4.0f,
        static_cast<float>(height_) / 4.0f
    );

    {
        rawOutput_->bind();
        gd_.setClearColor(1.f, 1.f, 1.f, 1.f);
        gd_.clearBuffers();

        ssaoPipeline_->use();

        gbAttachments[0]->bind(0);                          
        ssaoPipeline_->setSamplerUniform("gPosition", 0);

        gbAttachments[1]->bind(1);                          
        ssaoPipeline_->setSamplerUniform("gNormal", 1);

        noiseTexture_->bind(2);
        ssaoPipeline_->setSamplerUniform("u_noiseTexture", 2);

        ssaoPipeline_->setUniform("u_projection", glm::value_ptr(ctx.projection));
        ssaoPipeline_->setUniform("u_view",       glm::value_ptr(ctx.view));
        ssaoPipeline_->setUniform("u_noiseScale", glm::value_ptr(noiseScale));

        GLint samplesLoc = glGetUniformLocation(
            static_cast<GLuint>(ssaoPipeline_->id()), "u_samples");
        if (samplesLoc != -1) {
            glProgramUniform3fv(
                static_cast<GLuint>(ssaoPipeline_->id()),
                samplesLoc,
                KERNEL_SIZE,
                glm::value_ptr(samples_[0])
            );
        }

        drawQuad();
        rawOutput_->unbind();
    }

    {
        blurOutput_->bind();
        gd_.setClearColor(1.f, 1.f, 1.f, 1.f);
        gd_.clearBuffers();

        blurPipeline_->use();

        rawOutput_->getColorAttachments()[0]->bind(0);
        blurPipeline_->setSamplerUniform("u_ssaoInput", 0);

        drawQuad();
        blurOutput_->unbind();
    }

    glEndQuery(GL_TIME_ELAPSED);
}

void SSAOPass::resize(u32 w, u32 h)
{
  if (!rawOutput_ || !blurOutput_) return;
    width_  = w;
    height_ = h;
    rawOutput_->resize(w, h);
    blurOutput_->resize(w, h);
}

void SSAOPass::generateSamples()
{
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::default_random_engine gen(42u); 

    samples_.clear();
    samples_.reserve(KERNEL_SIZE);

    for (int i = 0; i < KERNEL_SIZE; ++i)
    {
        glm::vec3 sample(
            dist(gen) * 2.0f - 1.0f,   
            dist(gen) * 2.0f - 1.0f,   
            dist(gen)                   
        );
        sample = glm::normalize(sample);  
        sample *= dist(gen);              

        float t = static_cast<float>(i) / static_cast<float>(KERNEL_SIZE);
        sample *= glm::mix(0.1f, 1.0f, t * t);

        samples_.push_back(sample);
    }
}

void SSAOPass::generateNoiseTexture()
{
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::default_random_engine gen(137u); 

    std::vector<float> noiseData;
    noiseData.reserve(16 * 2); 
    for (int i = 0; i < 16; ++i) {
        noiseData.push_back(dist(gen)); 
        noiseData.push_back(dist(gen)); 
    }

    Texture::TextureSettings settings;
    settings.minFilter = Filter::NEAREST; 
    settings.magFilter = Filter::NEAREST;
    settings.wrap      = Wrap::REPEAT;    
    settings.format    = Format::RG32F;

    noiseTexture_ = std::make_unique<GLTexture>(4u, 4u, settings);
    noiseTexture_->setData(
        noiseData.data(),
        static_cast<u32>(noiseData.size() * sizeof(float)),
        GL_RG,
        GL_FLOAT
    );
}

void SSAOPass::initQuad()
{
    float verts[] = {
      -1.f,  1.f,  0.f, 1.f,
      -1.f, -1.f,  0.f, 0.f,
       1.f, -1.f,  1.f, 0.f,
      -1.f,  1.f,  0.f, 1.f,
       1.f, -1.f,  1.f, 0.f,
       1.f,  1.f,  1.f, 1.f,
    };
    quadVAO_ = gd_.createVertexArray();
    quadVAO_->bind();
    quadVBO_ = gd_.createVertexBuffer(nullptr, sizeof(verts));
    quadVBO_->uploadData(verts, sizeof(verts));
    quadVAO_->addVertexBuffer(
        quadVBO_.get(), 0, 2, UniformType::Float2, false,
        4 * sizeof(float), nullptr);
    quadVAO_->addVertexBuffer(
        quadVBO_.get(), 1, 2, UniformType::Float2, false,
        4 * sizeof(float), reinterpret_cast<const void*>(2 * sizeof(float)));
    quadVAO_->unBind();
}

void SSAOPass::drawQuad()
{
    quadVAO_->bind();
    gd_.draw(6u);
    quadVAO_->unBind();
}

} // namespace mam
