#pragma once

#include "common/common.hpp"

namespace mam {

  class MaterialModule;
  class MaterialModuleRegistry;

  /**
   * @brief Target graphics API for shader source generation.
   */
  enum class ShaderBackendTarget {
    OpenGL = 0, ///< Generate GLSL for OpenGL 4.6.
    Vulkan = 1  ///< Generate GLSL for Vulkan (SPIR-V).
  };

  /**
   * @brief Result of a shader composition pass.
   *
   * Contains the generated vertex and fragment GLSL source strings, as well
   * as any errors or warnings produced during composition.
   */
  struct CompositionResult {
    std::string              vertexShader;   ///< Generated vertex shader GLSL source.
    std::string              fragmentShader; ///< Generated fragment shader GLSL source.
    std::vector<std::string> errors;         ///< Error messages (non-empty on failure).
    std::vector<std::string> warnings;       ///< Warning messages (non-fatal).
    bool                     isSuccessful;   ///< True if composition completed without errors.

    CompositionResult() : isSuccessful(false) {}
  };

  /**
   * @brief Assembles a complete shader program from an ordered list of material modules.
   *
   * The composer resolves dependencies between modules, sorts them
   * topologically by stage and priority, merges their uniforms/inputs/outputs
   * and code sections, and fills a GLSL template to produce final source strings.
   *
   * Use setVertexTemplate() and setFragmentTemplate() to supply the base
   * template files, then call compose() with the module IDs to produce a
   * CompositionResult.
   */
  class ShaderComposer {
  public:
    /**
     * @brief Construct the composer bound to a module registry.
     * @param registry Module registry used to resolve module IDs.
     */
    ShaderComposer(MaterialModuleRegistry& registry);
    ~ShaderComposer() = default;

    /**
     * @brief Set the vertex shader template file path.
     * @param vertexPath Path to the vertex template file.
     */
    void setVertexTemplate(const std::string& vertexPath);

    /**
     * @brief Set the fragment shader template file path.
     * @param fragmentPath Path to the fragment template file.
     */
    void setFragmentTemplate(const std::string& fragmentPath);

    /**
     * @brief Set the target graphics API for code generation.
     * @param target Target API (OpenGL or Vulkan).
     */
    void setBackendTarget(ShaderBackendTarget target) { backendTarget_ = target; }

    /// Returns the currently configured backend target.
    ShaderBackendTarget backendTarget() const { return backendTarget_; }

    /**
     * @brief Compose a complete shader program from the given module IDs.
     * @param moduleIDs Ordered list of module IDs to include.
     * @return CompositionResult with the generated GLSL or error messages.
     */
    CompositionResult compose(const std::vector<ID>& moduleIDs);

  private:
    /// Intermediate grouping of modules by shader stage.
    struct ModulesByStage {
      std::vector<MaterialModule*> vertexModules;   ///< Modules targeting the vertex stage.
      std::vector<MaterialModule*> fragmentModules; ///< Modules targeting the fragment stage.
    };

    ModulesByStage separateModulesByStage(const std::vector<ID>& moduleIDs);

    std::vector<MaterialModule*> resolveAndSortModules(
      const std::vector<MaterialModule*>& modules,
      ShaderStage stage);

    bool topologicalSort(
      const std::vector<MaterialModule*>& modules,
      std::vector<MaterialModule*>& sorted,
      std::vector<std::string>& errors);

    std::string mergeUniforms(const std::vector<MaterialModule*>& modules);
    std::string mergeInputs(const std::vector<MaterialModule*>& modules);
    std::string mergeOutputs(const std::vector<MaterialModule*>& modules);
    std::string combineFunctions(const std::vector<MaterialModule*>& modules);
    std::string combineMainCode(const std::vector<MaterialModule*>& modules);

    bool validateRequirements(
      const std::vector<MaterialModule*>& modules,
      std::vector<std::string>& errors);

    std::string fillTemplate(
      const std::string& templateContent,
      const std::string& uniforms,
      const std::string& inputs,
      const std::string& outputs,
      const std::string& functions,
      const std::string& mainCode);

    std::string loadTemplateFile(const std::string& path);

    MaterialModuleRegistry& materialModuleRegistry;    ///< Module registry reference.
    std::string             vertexTemplatePath_;       ///< Path to the vertex template file.
    std::string             fragmentTemplatePath_;     ///< Path to the fragment template file.
    std::string             vertexTemplateContent_;    ///< Cached vertex template source.
    std::string             fragmentTemplateContent_;  ///< Cached fragment template source.
    ShaderBackendTarget     backendTarget_ = ShaderBackendTarget::OpenGL; ///< Active backend target.
  };

} // namespace mam
