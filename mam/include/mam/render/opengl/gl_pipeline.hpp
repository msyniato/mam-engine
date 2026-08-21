#pragma once

#include "render/api/pipeline.hpp"

namespace mam {

  class Texture;

  /**
   * @brief Represents a single OpenGL uniform variable
   */
  struct GLUniform {
    GLint location;      ///< Location of the uniform in the shader
    UniformType type;    ///< Type of the uniform (float, int, vec3, etc.)
  };

  /**
   * @brief OpenGL implementation of a rendering pipeline (Pipeline2)
   *
   * Manages shader compilation, linking, uniforms, and binding of the OpenGL pipeline.
   */
  class GLPipeline : public Pipeline
  {
  public:
    /**
     * @brief Construct a new GLPipeline2
     */
    GLPipeline();

    /**
     * @brief Destroy the GLPipeline2, releases OpenGL resources
     */
    virtual ~GLPipeline() override;

    /**
     * @brief Add a shader to the pipeline
     * @param shader Unique pointer to a Shader object
     */
    virtual void addShader(Shader* shader) override;

    /**
     * @brief Compile and link all shaders in the pipeline.
     * @return std::nullopt on success, or an error message string on failure.
     */
    virtual std::optional<std::string> create() override;

    /**
     * @brief Activate this pipeline for rendering
     */
    virtual void use() override;

    /**
     * @brief Get the location of a uniform variable by name
     * @param name Name of the uniform
     * @return Location in the shader program
     */
    virtual int getUniformLocation(const char* name) const override;

    /**
     * @brief Set a uniform value
     * @param name Uniform name
     * @param value Pointer to the value to set
     */
    virtual void setUniform(const char* name, const void* value) override;

    /**
     * @brief Set a sampler uniform to a texture unit
     * @param name Uniform name
     * @param unit Texture unit index
		 */
    void setSamplerUniform(const char* name, int unit) override;

    /**
     * @brief Set a sampler uniform to a texture unit
     * @param name Uniform name
     * @param unit Texture unit index
		 */
    virtual void bindTexture(const Texture* tex, u32 set, u32 binding) override {}

    /**
     * @brief Get the OpenGL ID of this pipeline
     * @return GLuint ID
     */
    virtual u32 id() const override;

  private:
    GLuint id_; ///< OpenGL program ID

    std::unordered_map<std::string, GLUniform> uniforms; ///< Cached uniforms
    std::vector<std::string> uniformNames;              ///< Ordered list of uniform names
  };

}