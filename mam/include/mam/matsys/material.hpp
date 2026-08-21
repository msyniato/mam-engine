#pragma once

#include "common/common.hpp"

namespace mam {

  class Pipeline;
  class ShaderComposer;
  class MaterialModuleRegistry;
  class GraphicsDevice;

  /**
   * @brief Variant type that holds any scalar or vector material parameter value.
   *
   * Supported types: bool, int, float, vec2, vec3, vec4, mat4.
   */
  using MaterialParamValue = std::variant<
    bool,
    int,
    float,
    glm::vec2,
    glm::vec3,
    glm::vec4,
    glm::mat4
  >;

  /**
   * @brief A single named, typed parameter that belongs to a material.
   *
   * Parameters are collected from material modules during compilation.
   * They can be overridden per-material instance via Material::setParameter().
   */
  struct MaterialParameter {
    std::string        name;           ///< Parameter name as declared in the shader module.
    std::string        type;           ///< GLSL type string (e.g. "float", "vec3").
    MaterialParamValue value;          ///< Current parameter value.
    ID                 sourceModuleID; ///< ID of the module that declared this parameter.
    bool               isOverridden;  ///< True if the value has been set by the material instance.

    MaterialParameter()
      : sourceModuleID(0), isOverridden(false), value(0.0f) {}
  };

  /**
   * @brief Plain-data descriptor used to create a Material.
   *
   * Stores the list of module IDs to compose, a human-readable name,
   * and an auto-assigned ID.
   */
  struct MaterialData {
    std::vector<ID> moduleIDs; ///< Ordered list of module IDs to compose into this material.
    std::string     name;      ///< Human-readable material name.
    ID              id;        ///< Unique material identifier assigned by MaterialRegistry.

    MaterialData() : id(0) {}
  };

  /**
   * @brief A compiled, bindable material composed from one or more shader modules.
   *
   * Material owns a ShaderComposer and a compiled Pipeline. Parameters are
   * collected from all constituent modules and can be overridden per-instance.
   * Call compile() after construction; recompile when module source changes.
   */
  class Material {
  public:
    /**
     * @brief Construct the material from a descriptor, module registry, and device.
     * @param data     Material descriptor with module IDs and name.
     * @param registry Module registry used by the ShaderComposer.
     * @param gd       Graphics device used to create the Pipeline.
     */
    Material(const MaterialData& data, MaterialModuleRegistry& registry, GraphicsDevice& gd);
    ~Material() = default;

    /**
     * @brief Compile (or recompile) the material's shader pipeline.
     * @return True if compilation succeeded; false on error (see getLastError()).
     */
    bool compile();

    /// Returns true if the compiled pipeline is stale and needs recompilation.
    bool needsRecompile() const;

    /// Bind the material's pipeline for subsequent draw calls.
    void bind() const;

    /**
     * @brief Set a parameter value by name.
     * @tparam T  Value type (must match the parameter's declared GLSL type).
     * @param name Parameter name.
     * @param value New value.
     */
    template<typename T>
    void setParameter(const std::string& name, const T& value);

    /**
     * @brief Check whether a parameter with the given name exists.
     * @param name Parameter name.
     * @return True if the parameter is present.
     */
    inline bool hasParameter(const std::string& name) const {
      return parameters_.find(name) != parameters_.end();
    }

    /**
     * @brief Return a const pointer to the named parameter, or nullptr.
     * @param name Parameter name.
     * @return Pointer to the parameter, or nullptr if not found.
     */
    inline const MaterialParameter* getParameter(const std::string& name) const {
      auto it = parameters_.find(name);
      return it != parameters_.end() ? &it->second : nullptr;
    }

    /**
     * @brief Return all parameters as a flat vector.
     * @return Copy of all material parameters.
     */
    std::vector<MaterialParameter> getAllParameters() const;

    /// Upload all parameters to the bound pipeline's uniform locations.
    void uploadParameters() const;

    /// Returns the material's plain-data descriptor.
    const MaterialData& getData() const { return data_; }

    /// Returns the unique material ID.
    ID getID() const { return data_.id; }

    /// Returns the human-readable material name.
    const std::string& getName() const { return data_.name; }

    /// Returns the ordered list of module IDs used to compose this material.
    const std::vector<ID>& getModuleIDs() const { return data_.moduleIDs; }

    /// Returns a mutable shared pointer to the compiled pipeline.
    std::shared_ptr<Pipeline> getPipeline() { return pipeline_; }

    /// Returns an immutable shared pointer to the compiled pipeline.
    const std::shared_ptr<Pipeline> getPipeline() const { return pipeline_; }

    /// Returns the ShaderComposer that generated this material's source.
    ShaderComposer* getComposer() const { return composer_.get(); }

    /// Returns true if the material has been compiled at least once without error.
    bool isCompiled() const { return compiled_; }

    /// Returns the error string from the most recent failed compile(), or empty.
    const std::string& getLastError() const { return lastError_; }

  private:
    void collectParametersFromModules();
    MaterialParamValue parseDefaultValue(const std::string& type, const std::string& defaultValueStr);
    int  getUniformLocation(const std::string& name) const;
    void uploadParameter(const MaterialParameter& param) const;
    bool setParameterInternal(const std::string& name, UniformType expectedType, const MaterialParamValue& value);

    glm::vec2 parseVec2(const std::string& str);
    glm::vec3 parseVec3(const std::string& str);
    glm::vec4 parseVec4(const std::string& str);
    glm::mat4 parseMat4(const std::string& str);

    std::unordered_map<std::string, MaterialParameter> parameters_;            ///< Named parameter map.
    mutable std::unordered_map<std::string, int>       uniformLocationCache_;  ///< Cached uniform locations.
    std::unique_ptr<ShaderComposer>                    composer_;              ///< Shader source composer.
    std::shared_ptr<Pipeline>                          pipeline_;              ///< Compiled GPU pipeline.
    MaterialModuleRegistry&                            materialModuleRegistry_; ///< Module registry reference.
    GraphicsDevice&                                    graphicsDevice_;         ///< Graphics device reference.
    MaterialData                                       data_;                   ///< Material descriptor.
    std::string                                        lastError_;             ///< Last compilation error.
    bool                                               compiled_;              ///< Whether compilation succeeded.
  };

  /**
   * @brief Creates, stores, and manages the lifetime of Material instances.
   *
   * Materials can be created by module ID list or by module name list.
   * The registry also supports bulk recompilation when module source changes.
   */
  class MaterialRegistry {
  public:
    /**
     * @brief Construct the registry.
     * @param registry Module registry supplying shader modules.
     * @param gd       Graphics device used to compile material pipelines.
     */
    MaterialRegistry(MaterialModuleRegistry& registry, GraphicsDevice& gd);
    ~MaterialRegistry() = default;

    /**
     * @brief Set the GLSL template files used by the ShaderComposer.
     * @param vertexPath   Path to the vertex shader template.
     * @param fragmentPath Path to the fragment shader template.
     */
    void setShaderTemplatePaths(const std::string& vertexPath,
                                const std::string& fragmentPath);

    /**
     * @brief Create a material from an ordered list of module IDs.
     * @param name      Human-readable material name.
     * @param moduleIDs Ordered list of module IDs to compose.
     * @return Pointer to the new material.
     */
    Material* createMaterial(const std::string& name,
                             const std::vector<ID>& moduleIDs);

    /**
     * @brief Create a material from an ordered list of module names.
     * @param name        Human-readable material name.
     * @param moduleNames Ordered list of module names to compose.
     * @return Pointer to the new material.
     */
    Material* createMaterialFromModuleNames(const std::string& name,
                                            const std::vector<std::string>& moduleNames);

    /**
     * @brief Look up a material by its ID.
     * @param id Material identifier.
     * @return Pointer to the material, or nullptr if not found.
     */
    Material* getMaterial(ID id);

    /// @copydoc getMaterial(ID)
    const Material* getMaterial(ID id) const;

    /**
     * @brief Look up a material by its name.
     * @param name Material name.
     * @return Pointer to the material, or nullptr if not found.
     */
    Material* getMaterial(const std::string& name);

    /// @copydoc getMaterial(const std::string&)
    const Material* getMaterial(const std::string& name) const;

    /**
     * @brief Destroy the material with the given ID.
     * @param id Material identifier.
     */
    void destroyMaterial(ID id);

    /// Recompile every material in the registry.
    void recompileAllMaterials();

    /**
     * @brief Recompile all materials that reference a given module.
     * @param moduleID ID of the changed module.
     */
    void recompileMaterialsUsingModule(ID moduleID);

    /// Returns a const reference to the internal material map.
    const std::unordered_map<ID, std::unique_ptr<Material>>& getAllMaterials() const {
      return materials_;
    }

  private:
    std::unordered_map<ID, std::unique_ptr<Material>> materials_;            ///< Owned materials keyed by ID.
    std::unordered_map<std::string, ID>               nameToID_;             ///< Name → ID lookup.
    MaterialModuleRegistry&                           registry_;             ///< Module registry reference.
    GraphicsDevice&                                   graphicsDevice_;       ///< Graphics device reference.
    std::string                                       vertexTemplatePath_;   ///< Vertex template path.
    std::string                                       fragmentTemplatePath_; ///< Fragment template path.
    ID                                                nextMaterialID_;       ///< Next ID to assign.
  };

} // namespace mam
