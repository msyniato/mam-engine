
#include "matsys/material_module.hpp"
#include "core/utils.hpp"

namespace mam {
  
#pragma region shader_module_parser
  ParsedModule ShaderModuleParser::parse(const std::string& filePath) {
    ParsedModule result;
    result.filePath = filePath;
    
    std::vector<std::string> lines = readFileLines(filePath);
    if (!lines.empty())
    {
      result.metadata = parseMetadata(lines);
      result.parameters = parseParams(lines);
      result.code = parseCode(lines);
    } 
    
    return result;
  }
  
  std::vector<std::string> ShaderModuleParser::readFileLines(const std::string& filePath) {
    std::vector<std::string> lines;
    
    std::ifstream file(filePath);
    
    if (file.is_open())
    {
      std::string line;
      while (std::getline(file, line))
      {
        lines.push_back(line);
      }
      
      file.close();
    }
    
    return lines;
  }
  
  ModuleMetadata ShaderModuleParser::parseMetadata(const std::vector<std::string>& lines) {
    ModuleMetadata metadata;
    
    for (const auto& line :lines)
    {
      std::string trimmed = trim(line);
      if (trimmed.size() < 2 || trimmed[0] != '/' || trimmed[1] != '/') continue;
      
      std::string content = trimmed.substr(2);
      content = trim(content);
      
      size_t colonPos = content.find(':');
      if (colonPos == std::string::npos) continue;
      
      std::string key = trim(content.substr(0, colonPos));
      std::string value = trim(content.substr(colonPos + 1));
      
      if (key == "name") {
        metadata.name = value; 
      } 
      else if (key == "stage") {
        if (value == "vertex") metadata.stage = ShaderStage::Vertex;
        else if (value == "fragment") metadata.stage = ShaderStage::Fragment;
      } 
      else if (key == "dependencies") {
        metadata.dependencies = parseStringArray(value);
      } 
      else if (key == "priority") {
        metadata.priority = std::stoi(value);
      }
      else if (key == "description") {
        metadata.description = value;
      }
      
    }
    
    return metadata;
  }
  
  std::vector<std::string> ShaderModuleParser::parseStringArray(const std::string& str) {
    std::vector<std::string> result;
    
    std::string cleaned = str;
    cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), '['), cleaned.end());
    cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), ']'), cleaned.end());
    
    cleaned = trim(cleaned);
    if (!cleaned.empty()) {
      std::stringstream ss(cleaned);
      std::string item;
      while (std::getline(ss, item, ',')) {
        item = trim(item);
        if (!item.empty()) {
          result.push_back(item);
        }
      }
    }
    
    return result;
  }
  
  std::vector<ModuleParameter> ShaderModuleParser::parseParams(const std::vector<std::string>& lines) {
    std::vector<ModuleParameter> params;
    
    bool inParamSection = false;
    
    for (const auto& line : lines) {
      std::string trimmed = trim(line);
      
      if (trimmed.find("// PARAMETERS") != std::string::npos) {
        inParamSection = true;
        continue;
      }
      if (trimmed.find("// FUNCTIONS") != std::string::npos || 
          trimmed.find("// MAIN_CODE") != std::string::npos) {
        inParamSection = false;
        continue;
          }
      
      if (!inParamSection) {
        continue;
      }
      
      if (trimmed.size() < 2 || trimmed[0] != '/' || trimmed[1] != '/') {
        continue;
      }
      
      std::string content = trimmed.substr(2);
      content = trim(content);
        
      if (content.empty()) {
        continue;
      }
      
      ModuleParameter param = parseParamDeclaration(content);
      if (!param.name.empty()) {
        params.push_back(param);
      }
    }
    
    return params;
  }
  
  ModuleParameter ShaderModuleParser::parseParamDeclaration(const std::string& declaration) {
    ModuleParameter param;
    
    std::stringstream ss(declaration);
    std::string categoryStr, typeStr, nameStr;
    
    ss >> categoryStr;
    
    
    if (categoryStr == "uniform") {
      param.category = ModuleParameter::Category::Uniform;
    } 
    else if (categoryStr == "input") {
      param.category = ModuleParameter::Category::Input;
    } 
    else if (categoryStr == "output") {
      param.category = ModuleParameter::Category::Output;
    } 
    else if (categoryStr == "requires") {
      param.category = ModuleParameter::Category::Requires;
    }
    else {
      return param;
    }
    
    ss >> typeStr >> nameStr;
    
    param.type = typeStr;
    param.name = nameStr;
    
    std::string remainder;
    std::getline(ss, remainder);
    
    size_t colonPos = remainder.find(':');
    if (colonPos != std::string::npos) {
      param.defaultValue = trim(remainder.substr(colonPos + 1));
    }
    
    return param;
  }
  
  ModuleCode ShaderModuleParser::parseCode(const std::vector<std::string>& lines) {
    ModuleCode code;
    
    code.functions = extractSection(lines, "// FUNCTIONS", "// MAIN_CODE");
    code.mainCode = extractSection(lines, "// MAIN_CODE", "");
    
    return code;
  }
  
  std::string ShaderModuleParser::extractSection(const std::vector<std::string>& lines,
                                                 const std::string& startMarker,
                                                 const std::string& endMarker)
  {
    std::stringstream result;
    bool inSection = false;
    
    for (const auto& line : lines)
    {
      if (line.find(startMarker) != std::string::npos)
      {
        inSection = true;
        continue;
      }
      
      if (!endMarker.empty() && line.find(endMarker) != std::string::npos)
      {
        break;
      }
      
      if (inSection)
      {
        std::string trimmed = trim(line);
        if (trimmed.empty() || (trimmed.size() >= 2 && trimmed[0] == '/' && trimmed[1] == '/'))
        {
          continue;
        }
        result << line << "\n";
        
      }
    }
    
    return result.str();
  }
#pragma endregion
  
#pragma region material_module
  MaterialModule::MaterialModule(ID id, const std::string& filePath) :
    id_(id),
    filePath_(filePath),
    isLoaded_(false)
  {
    
  }
  
  bool MaterialModule::load()
  {
    
    parsedModule_ = ShaderModuleParser::parse(filePath_);

    if (parsedModule_.metadata.name.empty())
    {
      isLoaded_ = false;
      return false;
    }
    
    isLoaded_ = true;
    return true;
  }

  bool MaterialModule::reload()
  {
    return load();
  }
#pragma endregion
  
#pragma region material_module_registry
  ID MaterialModuleRegistry::registerModule(const std::string& filePath)
  {
    ID newID = nextID_++;
    auto module = std::make_unique<MaterialModule>(newID, filePath);
    
    if (!module->load())
    {
      return kInvalidID;
    }
    
    const std::string& moduleName = module->getName();
    if (nameToID_.find(moduleName) != nameToID_.end())
    {
      return kInvalidID;
    }
    
    nameToID_[moduleName] = newID;
    modules_[newID] = std::move(module);
    
    return newID;
  }

  bool MaterialModuleRegistry::reloadModule(ID id)
  {
    auto it = modules_.find(id);
    if (it == modules_.end())
    {
      return false;
    }
    
    return it->second->reload();
  }

  MaterialModule* MaterialModuleRegistry::getModule(ID id)
  {
    auto it = modules_.find(id);
    return (it != modules_.end()) ? it->second.get() : nullptr; 
  }
  
  const MaterialModule* MaterialModuleRegistry::getModule(ID id) const 
  {
    auto it = modules_.find(id);
    return (it != modules_.end()) ? it->second.get() : nullptr; 
  }

  MaterialModule* MaterialModuleRegistry::getModule(const std::string& name)
  {
    auto it = nameToID_.find(name);
    if (it == nameToID_.end())
    {
      return nullptr;
    }
    
    return getModule(it->second);
  }
  
  const MaterialModule* MaterialModuleRegistry::getModule(const std::string& name) const
  {
    auto it = nameToID_.find(name);
    if (it == nameToID_.end())
    {
      return nullptr;
    }
    
    return getModule(it->second);
  }

  void MaterialModuleRegistry::unloadModule(ID id)
  {
    auto it = modules_.find(id);
    if (it == modules_.end())
    {
      return;
    }
    
    const std::string& name = it->second->getName();
    nameToID_.erase(name);
    modules_.erase(it);
  }

  void MaterialModuleRegistry::clear()
  {
    modules_.clear();
    nameToID_.clear();
    nextID_ = 0;
  }
#pragma endregion
} // namespace mam
