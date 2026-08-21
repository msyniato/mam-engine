

#include "core/utils.hpp"

namespace mam
{
  glm::mat4 getModel(glm::vec3 p, glm::vec3 r, glm::vec3 s)
  {
    glm::mat4 model = glm::mat4(1.0f);

    model = glm::translate(model, p);

        model = glm::rotate(model, glm::radians(r.x), glm::vec3(1.0, 0.0, 0.0));
        model = glm::rotate(model, glm::radians(r.y), glm::vec3(0.0, 1.0, 0.0));
        model = glm::rotate(model, glm::radians(r.z), glm::vec3(0.0, 0.0, 1.0));

    model = glm::scale(model, s);

    return model;
  }
  
  std::string trim(const std::string& str)
  {
    if (str.empty()) return str;
    
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
      return "";
    }
    
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
  }
  
  UniformType stringToUniformType(const std::string& type) {
    if (type == "bool")        return UniformType::Bool;
    if (type == "int")         return UniformType::Int1;
    if (type == "float")       return UniformType::Float1;
    if (type == "vec2")        return UniformType::Float2;
    if (type == "vec3")        return UniformType::Float3;
    if (type == "vec4")        return UniformType::Float4;
    if (type == "mat4")        return UniformType::Mat4;
    return UniformType::Invalid;
  }
  
  std::string uniformTypeToString(UniformType type) {
    switch (type) {
    case UniformType::Bool:        return "bool";
    case UniformType::Int1:         return "int";
    case UniformType::Float1:       return "float";
    case UniformType::Float2:        return "vec2";
    case UniformType::Float3:        return "vec3";
    case UniformType::Float4:        return "vec4";
    case UniformType::Mat4:        return "mat4";
    default:                          return "invalid";
    }
  }
}
