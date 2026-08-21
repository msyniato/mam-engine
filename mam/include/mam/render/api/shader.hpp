#pragma once

#include "common/common.hpp"

namespace mam {

  /**
   * @brief Represents a GPU shader.
   */
  class Shader
  {
  public:
    /**
     * @brief Shader type enumeration.
     */
    enum Type {
      Invalid = 0, ///< Invalid shader
      Fragment,    ///< Fragment/pixel shader
      Vertex,      ///< Vertex shader
      Geometry     ///< Geometry shader
    };

    /// Destructor
    virtual ~Shader() {};

    /**
     * @brief Load shader source from memory.
     * @param shader_type Type of shader (Vertex, Fragment, etc.)
     * @param source Pointer to source code string
     * @param source_size Size of the source in bytes
     */
    virtual void loadSource(const Type shader_type,
      const char* source,
      const unsigned int source_size) = 0;

    /**
     * @brief Load shader source from file.
     * @param shader_type Type of shader
     * @param path Path to shader file
     */
    virtual void loadSourceFromFile(const Type shader_type,
      const char* path) = 0;

    /**
     * @brief Compile the shader.
     * @return True if compilation succeeded
     */
    virtual bool compile() = 0;

    /**
     * @brief Check if the shader has been compiled successfully.
     * @return True if compiled
     */
    virtual bool is_compiled() const = 0;

    /**
     * @brief Get the shader type.
     * @return Shader type
     */
    virtual Type type() const = 0;

    /**
     * @brief Get the GPU shader ID.
     * @return Shader ID
     */
    virtual uint32_t id() const = 0;

  protected:
    uint32_t id_;         ///< GPU handle/ID for the shader
    Type type_;           ///< Shader type
    bool is_compiled_;    ///< Compilation status
  };

} // namespace mam