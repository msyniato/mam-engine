
#include "matsys/material.hpp"
#include "matsys/shader_composer.hpp"
#include "matsys/material_module.hpp"
#include "render/api/pipeline.hpp"
#include "render/api/shader.hpp"
#include "core/utils.hpp"
#include "render/api/graphics_device.hpp"
#include "render/vulkan/vk_device.hpp"
#include "render/vulkan/vk_pipeline.hpp"

namespace mam {

  Material::Material(const MaterialData& data, MaterialModuleRegistry& registry, GraphicsDevice& gd) : 
  data_(data),
  compiled_(false),
  materialModuleRegistry_(registry),
  graphicsDevice_(gd)
  {
    composer_ = std::make_unique<ShaderComposer>(registry);
  }

  bool Material::compile()
  {
    compiled_ = false;
    lastError_.clear();
    uniformLocationCache_.clear();
    pipeline_.reset();

    if (dynamic_cast<VKDevice*>(&graphicsDevice_) != nullptr) {
      composer_->setBackendTarget(ShaderBackendTarget::Vulkan);
    } else {
      composer_->setBackendTarget(ShaderBackendTarget::OpenGL);
    }

    auto compositionResult = composer_->compose(data_.moduleIDs);
    
    if (!compositionResult.isSuccessful)
    {
      lastError_ = "Shader composition failed!";
      for (const auto& error : compositionResult.errors)
        lastError_ += "\n - " + error;
      return false;
    }
    
    auto vShader = graphicsDevice_.createShader();
    auto fShader = graphicsDevice_.createShader();
    
    if (!vShader || !fShader)
    {
      lastError_ = "Failed to create shader objects!";
      return false;
    }
    
    vShader->loadSource(
      Shader::Type::Vertex,
      compositionResult.vertexShader.c_str(),
      static_cast<unsigned int>(compositionResult.vertexShader.size())
    );

    fShader->loadSource(
      Shader::Type::Fragment,
      compositionResult.fragmentShader.c_str(),
      static_cast<unsigned int>(compositionResult.fragmentShader.size())
    );
    
    if (!vShader->compile() || !fShader->compile())
    {
      lastError_ = "Shader compilation failed!";
      return false;
    }
    
    pipeline_ = graphicsDevice_.createPipeline();
    if (!pipeline_)
    {
      lastError_ = "Failed to create pipeline object!";
      return false;
    }
    
    pipeline_->addShader(vShader.get());
    pipeline_->addShader(fShader.get());
    
    if (auto err = pipeline_->create(); err.has_value())
    {
      lastError_ = err.value();
      return false;
    }
    /*
    if (auto* vkDevice = dynamic_cast<VKDevice*>(&graphicsDevice_)) {
      auto vkPipeline = std::dynamic_pointer_cast<VKPipeline>(pipeline_);
      if (vkPipeline) vkDevice->registerPipeline(vkPipeline);
    }
    */

    collectParametersFromModules();
    auto comp = composer_->compose(data_.moduleIDs);

    compiled_ = true;
    return true;
  }

  void Material::collectParametersFromModules()
  {
    parameters_.clear();
    
    for (ID id : data_.moduleIDs)
    {
      const MaterialModule* module = materialModuleRegistry_.getModule(id);
      if (!module || !module->isLoaded()) continue;
      
      for (const auto& modParam : module->getParameters())
      {
        if (modParam.category != ModuleParameter::Category::Uniform)
          continue;
        
        if (parameters_.count(modParam.name))
          continue;
        
        MaterialParameter matParam;
        matParam.name = modParam.name;
        matParam.type = modParam.type;
        auto uniformType = stringToUniformType(modParam.type);
        if (uniformType == UniformType::Invalid) {
          continue;
        }
        matParam.sourceModuleID = id;
        matParam.isOverridden = false;
        
        matParam.value = parseDefaultValue(matParam.type, modParam.defaultValue);
        parameters_[modParam.name] = matParam;
      }
    }
  }

  MaterialParamValue Material::parseDefaultValue(const std::string& type, const std::string& defaultValueStr)
  {
    std::string str = trim(defaultValueStr);

    if (str.empty() || str == "null") {

      if (type == "bool")       return false;
      else if (type == "int")   return 0;
      else if (type == "float") return 0.0f;
      else if (type == "vec2")  return glm::vec2(0.0f);
      else if (type == "vec3")  return glm::vec3(0.0f);
      else if (type == "vec4")  return glm::vec4(0.0f);
      else if (type == "mat4")  return glm::mat4(1.0f);
      
      return 0.0f;
    }

    try
    {
      if (type == "bool")       return str == "true" || str == "1";
      else if (type == "int")   return std::stoi(str);
      else if (type == "float") return std::stof(str);
      else if (type == "vec2")  return parseVec2(str);
      else if (type == "vec3")  return parseVec3(str);
      else if (type == "vec4")  return parseVec4(str);
      else if (type == "mat4")  return parseMat4(str);

      return 0.0f;
    }
    catch (...)
    {
      if (type == "vec2")       return glm::vec2(0.0f);
      else if (type == "vec3")  return glm::vec3(0.0f);
      else if (type == "vec4")  return glm::vec4(0.0f);
      else if (type == "mat4")  return glm::mat4(1.0f);

      return 0.0f;
    }
  }
  
  static std::vector<float> parseFloatArray(const std::string& str) {
    std::vector<float> result;
    std::string cleaned = str;
    cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), '['), cleaned.end());
    cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), ']'), cleaned.end());

    std::stringstream ss(cleaned);
    std::string item;
    while (std::getline(ss, item, ',')) {
      item = trim(item);
      if (!item.empty())
        result.push_back(std::stof(item));
    }
    return result;
  }
  
  glm::vec2 Material::parseVec2(const std::string& str) {
    auto v = parseFloatArray(str);
    return v.size() >= 2 ? glm::vec2(v[0], v[1]) : glm::vec2(0.0f);
  }

  glm::vec3 Material::parseVec3(const std::string& str) {
    auto v = parseFloatArray(str);
    return v.size() >= 3 ? glm::vec3(v[0], v[1], v[2]) : glm::vec3(0.0f);
  }

  glm::vec4 Material::parseVec4(const std::string& str) {
    auto v = parseFloatArray(str);
    return v.size() >= 4 ? glm::vec4(v[0], v[1], v[2], v[3]) : glm::vec4(0.0f);
  }

  glm::mat4 Material::parseMat4(const std::string& str) {
    auto v = parseFloatArray(str);
    return v.size() >= 16 ? glm::make_mat4(v.data()) : glm::mat4(1.0f);
  }

  void Material::bind() const
  {
    if (!pipeline_)
      return;

    if (auto* vkDevice = dynamic_cast<VKDevice*>(&graphicsDevice_)) {
      vkDevice->setCurrentPipeline(pipeline_);
    }

    pipeline_->use();
  }

  void Material::uploadParameters() const
  {
    if (!pipeline_) return;
    for (const auto& [name, param] : parameters_)
      uploadParameter(param);
  }

  void Material::uploadParameter(const MaterialParameter& param) const
  {
    
    std::visit([this, &param](auto&& value) {
        using T = std::decay_t<decltype(value)>;
        
        const char* name = param.name.c_str();
        if constexpr (std::is_same_v<T, bool>) {
            int i = value ? 1 : 0;
            pipeline_->setUniform(name, &i);
        }
        else if constexpr (std::is_same_v<T, int>) {
            pipeline_->setUniform(name, &value);
        }
        else if constexpr (std::is_same_v<T, float>) {
            pipeline_->setUniform(name, &value);
        }
        else if constexpr (std::is_same_v<T, glm::vec2>) {
            pipeline_->setUniform(name, glm::value_ptr(value));
        }
        else if constexpr (std::is_same_v<T, glm::vec3>) {
            pipeline_->setUniform(name, glm::value_ptr(value));
        }
        else if constexpr (std::is_same_v<T, glm::vec4>) {
            pipeline_->setUniform(name, glm::value_ptr(value));
        }
        else if constexpr (std::is_same_v<T, glm::mat4>) {
            pipeline_->setUniform(name, glm::value_ptr(value));
        }
    }, param.value);
  }

  int Material::getUniformLocation(const std::string& name) const
  {
    auto it = uniformLocationCache_.find(name);
    if (it != uniformLocationCache_.end())
      return it->second;

    int loc = pipeline_ ? pipeline_->getUniformLocation(name.c_str()) : -1;
    uniformLocationCache_[name] = loc;
    return loc;
  }

  std::vector<MaterialParameter> Material::getAllParameters() const
  {
    std::vector<MaterialParameter> result;
    result.reserve(parameters_.size());
    for (const auto& [name, param] : parameters_)
      result.push_back(param);
    return result;
  }
  
  bool Material::setParameterInternal(const std::string& name, UniformType expectedType, const MaterialParamValue& value)
  {
    auto it = parameters_.find(name);
    
    if (it == parameters_.end()) {
      return false;
    }
    
    if (it->second.type != uniformTypeToString(expectedType)) {
      return false;
    }
    
    it->second.value = value;
    it->second.isOverridden = true;
    return true;
  }
  
  template<> void Material::setParameter(const std::string& name, const bool& value)      { setParameterInternal(name, UniformType::Bool,  value); }
  template<> void Material::setParameter(const std::string& name, const int& value)       { setParameterInternal(name, UniformType::Int1,   value); }
  template<> void Material::setParameter(const std::string& name, const float& value)     { setParameterInternal(name, UniformType::Float1, value); }
  template<> void Material::setParameter(const std::string& name, const glm::vec2& value) { setParameterInternal(name, UniformType::Float2,  value); }
  template<> void Material::setParameter(const std::string& name, const glm::vec3& value) { setParameterInternal(name, UniformType::Float3,  value); }
  template<> void Material::setParameter(const std::string& name, const glm::vec4& value) { setParameterInternal(name, UniformType::Float4,  value); }
  template<> void Material::setParameter(const std::string& name, const glm::mat4& value) { setParameterInternal(name, UniformType::Mat4,  value); }
  
  MaterialRegistry::MaterialRegistry(MaterialModuleRegistry& registry, GraphicsDevice& gd)
    : registry_(registry),
    graphicsDevice_(gd),
    nextMaterialID_(0)
  {

  }

  void MaterialRegistry::setShaderTemplatePaths(const std::string& vertexPath, const std::string& fragmentPath)
  {
    vertexTemplatePath_ = vertexPath;
    fragmentTemplatePath_ = fragmentPath;
  }

  Material* MaterialRegistry::createMaterial(const std::string& name, const std::vector<ID>& moduleIDs)
  {
    if (nameToID_.count(name))
    {
      return getMaterial(name);
    }

    MaterialData data;
    data.id = nextMaterialID_++;
    data.name = name;
    data.moduleIDs = moduleIDs;

    auto material = std::make_unique<Material>(data, registry_,graphicsDevice_);

    material->getComposer()->setVertexTemplate(vertexTemplatePath_);
    material->getComposer()->setFragmentTemplate(fragmentTemplatePath_);
    
    if (!material->compile())
    {
      return nullptr;
    }

    Material* ptr = material.get();
    nameToID_[name] = data.id;
    materials_[data.id] = std::move(material);

    return ptr;
  }

  Material* MaterialRegistry::createMaterialFromModuleNames(const std::string& name, const std::vector<std::string>& moduleNames)
  {
    std::vector<ID> moduleIDs;
    moduleIDs.reserve(moduleNames.size());

    for (const auto& moduleName : moduleNames) {
      MaterialModule* module = registry_.getModule(moduleName);
      if (!module) {
        return nullptr;
      }
      moduleIDs.push_back(module->getID());
    }

    return createMaterial(name, moduleIDs);
  }

  Material* MaterialRegistry::getMaterial(ID id)
  {
    auto it = materials_.find(id);
    return it != materials_.end() ? it->second.get() : nullptr;
  }

  const Material* MaterialRegistry::getMaterial(ID id) const
  {
    auto it = materials_.find(id);
    return it != materials_.end() ? it->second.get() : nullptr;
  }

  Material* MaterialRegistry::getMaterial(const std::string& name)
  {
    auto it = nameToID_.find(name);
    if (it == nameToID_.end()) return nullptr;

    return getMaterial(it->second);
  }

  const Material* MaterialRegistry::getMaterial(const std::string& name) const
  {
    auto it = nameToID_.find(name);
    if (it == nameToID_.end()) return nullptr;

    return getMaterial(it->second);
  }

  void MaterialRegistry::destroyMaterial(ID id)
  {
    auto it = materials_.find(id);
    if (it == materials_.end()) return;

    nameToID_.erase(it->second->getName());
    materials_.erase(it);
  }

  void MaterialRegistry::recompileAllMaterials()
  {
    for (auto& [id, material] : materials_) {
      material->getComposer()->setVertexTemplate(vertexTemplatePath_);
      material->getComposer()->setFragmentTemplate(fragmentTemplatePath_);
      material->compile();
    }
  }

  void MaterialRegistry::recompileMaterialsUsingModule(ID moduleID)
  {
    for (auto& [id, material] : materials_) {
      const auto& moduleIDs = material->getModuleIDs();
      bool uses = std::find(moduleIDs.begin(), moduleIDs.end(), moduleID) != moduleIDs.end();
      if (uses) {
        material->getComposer()->setVertexTemplate(vertexTemplatePath_);
        material->getComposer()->setFragmentTemplate(fragmentTemplatePath_);
        material->compile();
      }
    }
  }

} // namespace mam
