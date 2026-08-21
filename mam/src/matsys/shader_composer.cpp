#include "matsys/shader_composer.hpp"
#include "matsys/material_module.hpp"
namespace mam
{
  
  ShaderComposer::ShaderComposer(MaterialModuleRegistry& registry) :
  materialModuleRegistry(registry), 
  vertexTemplatePath_(""),
  vertexTemplateContent_(""),
  fragmentTemplatePath_(""),
  fragmentTemplateContent_("") {}

  void ShaderComposer::setFragmentTemplate(const std::string& fragmentPath)
  {
    fragmentTemplatePath_ = fragmentPath;
    fragmentTemplateContent_ = loadTemplateFile(fragmentPath);
  }

  void ShaderComposer::setVertexTemplate(const std::string& vertexPath)
  {
    vertexTemplatePath_ = vertexPath;
    vertexTemplateContent_ = loadTemplateFile(vertexPath);
  }

  std::string ShaderComposer::loadTemplateFile(const std::string& path)
  {
    std::ifstream file(path);
    if (!file.is_open()) {
      std::cerr << "ShaderComposer: failed to open template file: "
        << path << std::endl;
      return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return buffer.str();
  }

  CompositionResult ShaderComposer::compose(const std::vector<ID>& moduleIDs)
  {
    CompositionResult result;
    result.isSuccessful = false;
    
    ModulesByStage modules = separateModulesByStage(moduleIDs);
    
    if (!modules.vertexModules.empty())
    {
      auto vertexModules = resolveAndSortModules(modules.vertexModules, ShaderStage::Vertex);
        
      if (vertexModules.empty() && !modules.vertexModules.empty()) {
        result.errors.push_back("Vertex shader dependency resolution failed");
        return result;
      }
      
      if (!validateRequirements(vertexModules, result.errors)) {
        return result;
      }
      
      std::string uniforms = mergeUniforms(vertexModules);
      std::string inputs = mergeInputs(vertexModules);
      std::string outputs = mergeOutputs(vertexModules);
      
      std::string functions = combineFunctions(vertexModules);
      std::string mainCode = combineMainCode(vertexModules);
      
      result.vertexShader = fillTemplate(
          vertexTemplateContent_,
          uniforms, inputs, outputs, functions, mainCode
      );
    }
    
    if (!modules.fragmentModules.empty()) {
      auto fragmentModules = resolveAndSortModules(modules.fragmentModules, ShaderStage::Fragment);
        
      if (fragmentModules.empty() && !modules.fragmentModules.empty()) {
        result.errors.push_back("Fragment shader dependency resolution failed");
        return result;
      }
      
      if (!validateRequirements(fragmentModules, result.errors)) {
        return result;
      }
      
      std::string uniforms = mergeUniforms(fragmentModules);
      std::string inputs = mergeInputs(fragmentModules);
      std::string outputs = mergeOutputs(fragmentModules);
      
      std::string functions = combineFunctions(fragmentModules);
      std::string mainCode = combineMainCode(fragmentModules);
      
      result.fragmentShader = fillTemplate(
          fragmentTemplateContent_,
          uniforms, inputs, outputs, functions, mainCode
      );
    }
    
    
    result.isSuccessful = true;
    return result;
  }

  ShaderComposer::ModulesByStage ShaderComposer::separateModulesByStage(const std::vector<ID>& moduleIDs)
  {
    ModulesByStage result;
    
    for (ID moduleID : moduleIDs) {
      MaterialModule* module = materialModuleRegistry.getModule(moduleID);
      if (!module || !module->isLoaded()) {
        continue;
      }
        
      ShaderStage stage = module->getStage();
        
      if (stage == ShaderStage::Vertex) {
        result.vertexModules.push_back(module);
      } 
      else if (stage == ShaderStage::Fragment) {
        result.fragmentModules.push_back(module);
      }
      else {
      }
    }
    
    return result;
  }

  std::vector<MaterialModule*> ShaderComposer::resolveAndSortModules(const std::vector<MaterialModule*>& modules, ShaderStage stage)
  {
    std::vector<MaterialModule*> sorted;
    std::vector<std::string> errors;
    
    
    if (!topologicalSort(modules, sorted, errors)) {
     
      return {};  
    }
    
    
    return sorted;
  }

  bool ShaderComposer::topologicalSort(const std::vector<MaterialModule*>& modules, std::vector<MaterialModule*>& sorted, std::vector<std::string>& errors)
  {
    std::unordered_map<std::string, MaterialModule*> moduleMap;
    std::unordered_map<std::string, int> inDegree;  
    std::unordered_map<std::string, std::vector<std::string>> dependents;
    
    for (MaterialModule* module : modules) {
        const std::string& name = module->getName();
        moduleMap[name] = module;
        inDegree[name] = 0;
    }
    
    for (MaterialModule* module : modules) {
        const std::string& name = module->getName();
        const auto& deps = module->getMetadata().dependencies;
        
        for (const std::string& dep : deps) {

            if (moduleMap.find(dep) == moduleMap.end()) {
                errors.push_back("Module '" + name + "' depends on '" + dep + "' which is not included");
                return false;
            }
          
            inDegree[name]++;
          
            dependents[dep].push_back(name);
        }
    }
    
    std::queue<std::string> queue;
    for (MaterialModule* module : modules) {
        const std::string& name = module->getName();
        if (inDegree[name] == 0) {
            queue.push(name);
        }
    }
    
    while (!queue.empty()) {
        std::string current = queue.front();
        queue.pop();
      
        sorted.push_back(moduleMap[current]);
      
        for (const std::string& dependent : dependents[current]) {
            inDegree[dependent]--;
            
            if (inDegree[dependent] == 0) {
                queue.push(dependent);
            }
        }
    }
    
    if (sorted.size() != modules.size()) {
        errors.push_back("Circular dependency detected in modules");
        return false;
    }

    
    return true;
  }

  std::string ShaderComposer::mergeUniforms(const std::vector<MaterialModule*>& modules)
  {
    std::stringstream result;
    std::unordered_set<std::string> declared;

    if (backendTarget_ == ShaderBackendTarget::Vulkan) {
      result << "layout(set = 0, binding = 0) uniform MaterialUBO {\n";

      for (MaterialModule* module : modules) {
        const auto& params = module->getParameters();

        for (const auto& param : params) {
          if (param.category != ModuleParameter::Category::Uniform) continue;
          if (param.type.find("sampler") != std::string::npos) continue;
          if (declared.count(param.name)) continue;

          result << "  " << param.type << " " << param.name << ";\n";
          declared.insert(param.name);
        }
      }

      result << "};\n";

      constexpr u32 FIRST_TEXTURE_BINDING = 1;
      constexpr u32 MAX_TEXTURE_BINDINGS = 4;

      std::unordered_map<std::string, u32> samplerBindings = {
        {"u_texture0", FIRST_TEXTURE_BINDING + 0},
        {"u_texture1", FIRST_TEXTURE_BINDING + 1},
        {"u_texture2", FIRST_TEXTURE_BINDING + 2},
        {"u_texture3", FIRST_TEXTURE_BINDING + 3},
      };

      for (MaterialModule* module : modules) {
        const auto& params = module->getParameters();

        for (const auto& param : params) {
          if (param.category != ModuleParameter::Category::Uniform) continue;
          if (param.type.find("sampler") == std::string::npos) continue;
          if (declared.count(param.name)) continue;

          auto it = samplerBindings.find(param.name);
          if (it == samplerBindings.end()) continue;

          result << "layout(set = 0, binding = " << it->second << ") uniform "
            << param.type << " " << param.name << ";\n";

          declared.insert(param.name);
        }
      }

      return result.str();
    }

    for (MaterialModule* module : modules) {
      const auto& params = module->getParameters();

      for (const auto& param : params) {
        if (param.category != ModuleParameter::Category::Uniform)
          continue;

        if (declared.count(param.name))
          continue;

        result << "uniform " << param.type << " " << param.name << ";\n";
        declared.insert(param.name);
      }
    }

    return result.str();
  }

  std::string ShaderComposer::mergeInputs(const std::vector<MaterialModule*>& modules)
  {
    std::stringstream result;
    std::unordered_set<std::string> declared;

    static const std::unordered_map<std::string, int> s_locations = {
      {"a_position",  0},
      {"a_normal",    1},
      {"a_texCoord",  2},
      {"a_tangent",   3},
      {"a_instanceMatrix",   4},

      {"v_fragPos",   0},
      {"v_normal",    1},
      {"v_texCoord",  2},
      {"v_tangent",   3},
    };

    for (MaterialModule* module : modules) {
      for (const auto& param : module->getParameters()) {
        if (param.category != ModuleParameter::Category::Input)
          continue;

        if (declared.count(param.name))
          continue;

        auto it = s_locations.find(param.name);

        if (backendTarget_ == ShaderBackendTarget::Vulkan && it != s_locations.end()) {
          result << "layout(location = " << it->second << ") ";
        } else if (it != s_locations.end()) {
          result << "layout(location = " << it->second << ") ";
        }

        result << "in " << param.type << " " << param.name << ";\n";
        declared.insert(param.name);
      }
    }

    return result.str();
  }

  std::string ShaderComposer::mergeOutputs(const std::vector<MaterialModule*>& modules)
  {
    std::stringstream result;
    std::unordered_set<std::string> declared;

    static const std::unordered_map<std::string, int> s_locations = {
      {"v_fragPos",   0},
      {"v_normal",    1},
      {"v_texCoord",  2},
      {"v_tangent",   3},
      {"v_instanceMatrix",   4},
      {"FragColor",   0},
      {"outColor",    0},
    };

    for (MaterialModule* module : modules) {
      const auto& params = module->getParameters();

      for (const auto& param : params) {
        if (param.category != ModuleParameter::Category::Output)
          continue;

        if (declared.count(param.name))
          continue;

        auto it = s_locations.find(param.name);

        if (backendTarget_ == ShaderBackendTarget::Vulkan && it != s_locations.end()) {
          result << "layout(location = " << it->second << ") ";
        }

        result << "out " << param.type << " " << param.name << ";\n";
        declared.insert(param.name);
      }
    }

    return result.str();
  }

  std::string ShaderComposer::combineFunctions(const std::vector<MaterialModule*>& modules)
  {
    std::stringstream result;
    
    for (MaterialModule* module : modules) {
      const auto& code = module->getCode();
        
      if (!code.functions.empty()) {
        result << "// Functions from: " << module->getName() << "\n";
        result << code.functions;
        result << "\n";
      }
    }
    
    return result.str();
  }

  std::string ShaderComposer::combineMainCode(const std::vector<MaterialModule*>& modules)
  {
    std::stringstream result;
    
    for (MaterialModule* module : modules) {
      const auto& code = module->getCode();
        
      if (!code.mainCode.empty()) {
        result << "    // Code from: " << module->getName() << "\n";
        result << code.mainCode;
        result << "\n";
      }
    }
    
    return result.str();
  }

  bool ShaderComposer::validateRequirements(const std::vector<MaterialModule*>& modules, std::vector<std::string>& errors)
  {
    std::unordered_set<std::string> providedVariables;
    
    for (MaterialModule* module : modules) {
      const auto& params = module->getParameters();
      
      for (const auto& param : params) {
        if (param.category == ModuleParameter::Category::Requires) {
          if (providedVariables.find(param.name) == providedVariables.end()) {
            errors.push_back(
                "Module '" + module->getName() + 
                "' requires variable '" + param.name + 
                "' which is not provided by any previous module"
            );
            return false;
          }
        }
      }
      
      const std::string& mainCode = module->getCode().mainCode;
      
      std::istringstream iss(mainCode);
      std::string line;
      while (std::getline(iss, line)) {
        
        size_t pos = line.find('=');
        
        if (pos != std::string::npos) {
          std::string beforeEquals = line.substr(0, pos);
          std::istringstream declStream(beforeEquals);
          std::string type, name;
          declStream >> type >> name;
                
          if (!name.empty() && !type.empty()) {
            providedVariables.insert(name);
          }
        }
      }
    }
    
    return true;
  }

  std::string ShaderComposer::fillTemplate(const std::string& templateContent, const std::string& uniforms, const std::string& inputs, const std::string& outputs, const std::string& functions, const std::string& mainCode)
  {
    std::string result = templateContent;
    
    std::string marker = "// UNIFORMS_INJECTION_POINT";
    size_t pos = result.find(marker);
    if (pos != std::string::npos) {
      result.replace(pos, marker.length(), uniforms);
    }
    
    marker = "// INPUTS_INJECTION_POINT";
    pos = result.find(marker);
    if (pos != std::string::npos) {
      result.replace(pos, marker.length(), inputs);
    }
    
    marker = "// OUTPUTS_INJECTION_POINT";
    pos = result.find(marker);
    if (pos != std::string::npos) {
      result.replace(pos, marker.length(), outputs);
    }
    
    marker = "// FUNCTIONS_INJECTION_POINT";
    pos = result.find(marker);
    if (pos != std::string::npos) {
      result.replace(pos, marker.length(), functions);
    }
    
    marker = "// MAIN_CODE_INJECTION_POINT";
    pos = result.find(marker);
    if (pos != std::string::npos) {
      result.replace(pos, marker.length(), mainCode);
    }
    
    return result;
  }
}
