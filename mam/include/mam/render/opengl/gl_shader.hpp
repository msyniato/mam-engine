#pragma once

#include "render/api/shader.hpp"

namespace mam {

  /**
   * @brief OpenGL implementation of a Shader
   *
   * Handles loading, compiling, and querying OpenGL shader objects.
   */
  class GLShader : public Shader
  {
  public:
    /**
     * @brief Construct a new GLShader object
     */
    GLShader();

    /**
     * @brief Destroy the GLShader object
     */
    ~GLShader() override;

    /**
     * @brief Load shader source code from memory
     * @param shader_type Type of shader (Vertex, Fragment, Geometry)
     * @param source Pointer to shader source code
     * @param source_size Size of the source code in bytes
     */
    void loadSource(const Type shader_type,
      const char* source,
      const unsigned int source_size) override;

    /**
     * @brief Load shader source code from a file
     * @param shader_type Type of shader (Vertex, Fragment, Geometry)
     * @param path Path to the shader file
     */
    void loadSourceFromFile(const Type shader_type,
      const char* path) override;

    /**
     * @brief Compile the shader
     * @return true if compilation succeeded
     */
    bool compile() override;

    /**
     * @brief Check if the shader has been compiled successfully
     * @return true if compiled
     */
    bool is_compiled() const override;

    /**
     * @brief Get the type of the shader
     * @return Type enum (Vertex, Fragment, Geometry)
     */
    Type type() const override;

    /**
     * @brief Get the OpenGL shader ID
     * @return uint32_t OpenGL ID
     */
    uint32_t id() const override;

  };

}