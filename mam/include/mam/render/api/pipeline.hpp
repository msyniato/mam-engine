#pragma once

#include "common/common.hpp"

namespace mam {

  class Shader;
  class Texture;

  /**
   * @brief Abstract pipeline class for rendering with shaders.
   */
  class Pipeline
  {
  public:
    /// Default constructor
    Pipeline() {}

    /// Destructor
    virtual ~Pipeline() {}

    /**
     * @brief Add a shader to the pipeline.
     * @param shader Unique pointer to a Shader object
     */
    virtual void addShader(Shader* shader) = 0;

    /**
     * @brief Create the pipeline (compile/link shaders).
     * @return std::nullopt on success, or an error message string on failure.
     */
    virtual std::optional<std::string> create() = 0;

    /// Bind/use the pipeline for rendering
    virtual void use() = 0;

    /**
     * @brief Get the location of a uniform variable by name.
     * @param name Uniform name
     * @return Location of the uniform
     */
    virtual int getUniformLocation(const char* name) const = 0;

    /**
     * @brief Set a uniform value.
     * @param name Uniform name
     * @param value Pointer to the value
     */
    virtual void setUniform(const char* name, const void* value) = 0;
    
    /**
     * @brief Set a sampler uniform to a texture unit.
     * @param name Uniform name
     * @param unit Texture unit index
		 */
    virtual void setSamplerUniform(const char* name, int unit) = 0;

    /**
     * @brief Create a uniform block (optional override).
     * @param name Uniform block name
     * @param offset Offset in bytes
     * @param size Size in bytes
     */
    virtual void CreateUniform(std::string name, uint32_t offset, uint32_t size) {};

    virtual void bindTexture(const Texture* tex, u32 set, u32 binding) = 0;

    /**
     * @brief Get the pipeline ID (e.g., GPU handle)
     * @return Pipeline ID
     */
    virtual u32 id() const = 0;
  };

} // namespace mam