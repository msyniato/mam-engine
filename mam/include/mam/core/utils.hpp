#pragma once

#include "common/common.hpp"

namespace mam {

  /**
   * @brief Build a TRS (translate-rotate-scale) model matrix.
   * @param p World position.
   * @param r Euler rotation angles (radians).
   * @param s Scale factors.
   * @return Combined 4x4 model matrix.
   */
  glm::mat4 getModel(glm::vec3 p, glm::vec3 r, glm::vec3 s);

  /**
   * @brief Trim leading and trailing whitespace from a string.
   * @param str Input string.
   * @return Trimmed copy of the string.
   */
  std::string trim(const std::string& str);

  /**
   * @brief Convert a shader type name string to a UniformType enum value.
   * @param type Type name (e.g. "float", "vec3", "mat4").
   * @return Corresponding UniformType.
   */
  UniformType stringToUniformType(const std::string& type);

  /**
   * @brief Convert a UniformType enum value to its GLSL type name string.
   * @param type Uniform type enum.
   * @return GLSL type name string.
   */
  std::string uniformTypeToString(UniformType type);

  /**
   * @brief Compute an FNV-1a 64-bit hash of a null-terminated C string.
   * @param str Null-terminated input string.
   * @return 64-bit hash value.
   */
  constexpr inline u64 fnv1a(const char* str)
  {
    u64 hash = 14695981039346656037ull;
    while (*str)
      hash = (hash ^ u64(*str++)) * 1099511628211ull;
    return hash;
  }

  /**
   * @brief Compute an FNV-1a 64-bit hash of a string_view.
   * @param sv Input string view.
   * @return 64-bit hash value.
   */
  constexpr inline u64 fnv1a(std::string_view sv)
  {
    u64 hash = 14695981039346656037ull;
    for (char c : sv) {
      hash ^= static_cast<u64>(c);
      hash *= 1099511628211ull;
    }
    return hash;
  }

} // namespace mam
