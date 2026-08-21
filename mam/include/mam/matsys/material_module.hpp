#pragma once

#include "common/common.hpp"

namespace mam {

  /**
   * @brief A single parameter declared inside a shader module.
   *
   * Parameters describe uniforms, stage inputs, stage outputs, or
   * requirements imposed on other modules.
   */
  struct ModuleParameter {
    /**
     * @brief Semantic category of the parameter.
     */
    enum class Category {
      Uniform,  ///< GPU uniform variable.
      Input,    ///< Stage input (vertex attribute or interpolated varying).
      Output,   ///< Stage output (interpolated varying or fragment output).
      Requires  ///< Dependency declared by this module on another module.
    };

    std::string type;         ///< GLSL type string (e.g. "float", "vec3").
    std::string name;         ///< Parameter name.
    std::string defaultValue; ///< Default value string (may be empty).
    Category    category;     ///< Semantic category of this parameter.

    ModuleParameter() = default;

    /**
     * @brief Construct a fully specified parameter.
     * @param cat Semantic category.
     * @param t   GLSL type string.
     * @param n   Parameter name.
     * @param v   Default value string.
     */
    ModuleParameter(Category cat, std::string& t, std::string& n, std::string& v)
      : category(cat), type(t), name(n), defaultValue(v) {}
  };

  /**
   * @brief Header-level metadata parsed from a shader module file.
   *
   * Stores the list of module names this module depends on, the target
   * shader stage, the module name, a human-readable description, and a
   * priority used for topological sorting during composition.
   */
  struct ModuleMetadata {
    std::vector<std::string> dependencies; ///< Names of modules this module depends on.
    ShaderStage              stage;        ///< Target shader stage (vertex, fragment, etc.).
    std::string              name;         ///< Unique module name.
    std::string              description;  ///< Human-readable module description.
    s32                      priority;     ///< Composition priority — lower values are inserted earlier.

    ModuleMetadata() : stage(ShaderStage::Invalid), priority(100) {}
  };

  /**
   * @brief GLSL code sections parsed from a shader module file.
   *
   * Functions contains free GLSL function definitions; mainCode contains
   * the statements to inject into the shader's main() body.
   */
  struct ModuleCode {
    std::string functions; ///< Free GLSL function definitions contributed by this module.
    std::string mainCode;  ///< Code fragment injected into main().

    ModuleCode() = default;
  };

  /**
   * @brief Complete in-memory representation of a parsed shader module file.
   *
   * Aggregates the metadata, parameters, code sections, and original file path.
   */
  struct ParsedModule {
    ModuleMetadata               metadata;   ///< Module header metadata.
    std::vector<ModuleParameter> parameters; ///< All declared parameters.
    ModuleCode                   code;       ///< Parsed GLSL code sections.
    std::string                  filePath;   ///< Path to the source file.

    ParsedModule() = default;
  };

  /**
   * @brief Parses a .module shader file into a ParsedModule.
   *
   * All methods are static; ShaderModuleParser has no instance state.
   */
  class ShaderModuleParser {
  public:
    /**
     * @brief Parse the given file and return a fully populated ParsedModule.
     * @param filePath Path to the shader module file.
     * @return Parsed module data.
     */
    static ParsedModule parse(const std::string& filePath);

  private:
    static std::vector<std::string> readFileLines(const std::string& filePath);
    static ModuleMetadata           parseMetadata(const std::vector<std::string>& lines);
    static std::vector<ModuleParameter> parseParams(const std::vector<std::string>& lines);
    static ModuleCode               parseCode(const std::vector<std::string>& lines);
    static std::string              extractSection(const std::vector<std::string>& lines,
                                                   const std::string& startMarker,
                                                   const std::string& endMarker);
    static std::vector<std::string> split(const std::string& str, char delimiter);
    static std::vector<std::string> parseStringArray(const std::string& str);
    static ModuleParameter          parseParamDeclaration(const std::string& declaration);
  };

  /**
   * @brief A single loaded shader module backed by a ParsedModule.
   *
   * Wraps the parsed data and provides typed accessors. Call load() after
   * construction; call reload() to re-parse the file after it changes on disk.
   */
  class MaterialModule {
  public:
    /**
     * @brief Construct a module with the given ID and file path.
     * @param id       Unique module ID assigned by MaterialModuleRegistry.
     * @param filePath Path to the shader module source file.
     */
    MaterialModule(ID id, const std::string& filePath);
    ~MaterialModule() = default;

    /**
     * @brief Parse and load the module from its file path.
     * @return True if loading succeeded.
     */
    bool load();

    /**
     * @brief Re-parse the module file and update the in-memory data.
     * @return True if reloading succeeded.
     */
    bool reload();

    /// Returns the unique module ID.
    ID getID() const { return id_; }

    /// Returns the module name as declared in the file metadata.
    const std::string& getName() const { return parsedModule_.metadata.name; }

    /// Returns the path to the source file.
    const std::string& getFilePath() const { return filePath_; }

    /// Returns the complete parsed module data.
    const ParsedModule& getParsedData() const { return parsedModule_; }

    /// Returns the module metadata (stage, dependencies, priority, etc.).
    const ModuleMetadata& getMetadata() const { return parsedModule_.metadata; }

    /// Returns all parameters declared in this module.
    const std::vector<ModuleParameter>& getParameters() const { return parsedModule_.parameters; }

    /// Returns the GLSL code sections (functions + main code).
    const ModuleCode& getCode() const { return parsedModule_.code; }

    /// Returns true if the module has been successfully loaded.
    bool isLoaded() const { return isLoaded_; }

    /// Returns the target shader stage for this module.
    ShaderStage getStage() const { return parsedModule_.metadata.stage; }

  private:
    ID          id_;           ///< Unique module identifier.
    std::string filePath_;     ///< Path to the source file.
    ParsedModule parsedModule_; ///< Parsed in-memory representation.
    bool        isLoaded_;     ///< Whether load() has succeeded.
  };

  /**
   * @brief Owns and manages all MaterialModule instances.
   *
   * Modules are registered from file paths and assigned auto-generated IDs.
   * They can be looked up by ID or by name, reloaded individually, or unloaded.
   */
  class MaterialModuleRegistry {
  public:
    MaterialModuleRegistry() : nextID_(0) {}
    ~MaterialModuleRegistry() = default;

    /**
     * @brief Load and register a module from the given file path.
     * @param filePath Path to the shader module file.
     * @return The ID assigned to the registered module.
     */
    ID registerModule(const std::string& filePath);

    /**
     * @brief Reload the module with the given ID from its source file.
     * @param id Module identifier.
     * @return True if reloading succeeded.
     */
    bool reloadModule(ID id);

    /**
     * @brief Remove and destroy the module with the given ID.
     * @param id Module identifier.
     */
    void unloadModule(ID id);

    /**
     * @brief Look up a module by ID.
     * @param id Module identifier.
     * @return Mutable pointer to the module, or nullptr if not found.
     */
    MaterialModule* getModule(ID id);

    /**
     * @brief Look up a module by name.
     * @param name Module name as declared in the file metadata.
     * @return Mutable pointer to the module, or nullptr if not found.
     */
    MaterialModule* getModule(const std::string& name);

    /// @copydoc getModule(ID)
    const MaterialModule* getModule(ID id) const;

    /// @copydoc getModule(const std::string&)
    const MaterialModule* getModule(const std::string& name) const;

    /// Unload all modules and reset the registry.
    void clear();

    /// Returns a const reference to the internal module map.
    const std::unordered_map<ID, std::unique_ptr<MaterialModule>>& getModules() const {
      return modules_;
    }

  private:
    std::unordered_map<ID, std::unique_ptr<MaterialModule>> modules_;   ///< Owned modules keyed by ID.
    std::unordered_map<std::string, ID>                     nameToID_;  ///< Name → ID lookup.
    ID                                                       nextID_;    ///< Next ID to assign.
  };

} // namespace mam
