#pragma once

#include "render/api/shader.hpp"
#include <vulkan/vulkan.hpp>

namespace mam {

  class VKDevice;

  /**
   * @brief Implementación Vulkan de un shader.
   *
   * Envuelve un módulo de shader de Vulkan (`vk::ShaderModule`) y gestiona
   * la carga del código fuente, la compilación a SPIR-V y el tipo de etapa
   * de shader. Se utiliza como parte del pipeline gráfico de Vulkan.
   *
   * @note Esta clase no es copiable ni movible.
   * @see VKDevice
   */
  class VKShader : public Shader
  {
  public:
    /**
     * @brief Constructor.
     * @param device Puntero al dispositivo Vulkan lógico propietario de este shader.
     */
    VKShader(VKDevice* device);

    /**
     * @brief Destructor.
     *
     * Destruye el módulo de shader de Vulkan si fue creado.
     */
    ~VKShader() override;

    VKShader(const VKShader&) = delete;
    VKShader& operator=(const VKShader&) = delete;
    VKShader(VKShader&&) noexcept = delete;
    VKShader& operator=(VKShader&&) noexcept = delete;

    /**
     * @brief Carga el código fuente del shader desde memoria.
     *
     * Almacena el código fuente internamente para su posterior compilación
     * mediante @ref compile().
     *
     * @param shader_type Tipo de etapa de shader (Vertex, Fragment, etc.).
     * @param source      Puntero al código fuente del shader (GLSL o HLSL).
     * @param source_size Tamaño del código fuente en bytes.
     */
    void loadSource(const Type shader_type,
      const char* source,
      const unsigned int source_size) override;

    /**
     * @brief Carga el código fuente del shader desde un archivo en disco.
     *
     * Lee el archivo indicado y almacena su contenido internamente para su
     * posterior compilación mediante @ref compile().
     *
     * @param shader_type Tipo de etapa de shader.
     * @param path        Ruta al archivo con el código fuente del shader.
     */
    void loadSourceFromFile(const Type shader_type,
      const char* path) override;

    /**
     * @brief Compila el shader a SPIR-V y crea el módulo de Vulkan.
     *
     * Invoca el compilador de shaders (p. ej. shaderc) sobre el código fuente
     * cargado previamente, almacena el resultado en @ref spirvCache_ y crea
     * el objeto @ref shaderModule_ en el dispositivo Vulkan.
     *
     * @return @c true si la compilación fue exitosa; @c false en caso contrario.
     */
    bool compile() override;

    /**
     * @brief Indica si el shader ha sido compilado correctamente.
     * @return @c true si el módulo de shader es válido y está listo para su uso.
     */
    bool is_compiled() const override;

    /**
     * @brief Devuelve el tipo de etapa de este shader.
     * @return Enumerado @c Type que identifica la etapa (Vertex, Fragment, etc.).
     */
    Type type() const override;

    /**
     * @brief Devuelve el identificador interno del shader.
     *
     * En la implementación Vulkan este valor es dependiente de la
     * implementación y puede no tener un significado directo para el usuario.
     *
     * @return Identificador numérico del shader.
     */
    uint32_t id() const override;

    vk::ShaderModule shaderModule_;                    ///< Módulo de shader de Vulkan creado tras la compilación.
    vk::PipelineShaderStageCreateInfo shaderStage_;   ///< Información de etapa para la creación del pipeline gráfico.
    std::vector<uint32_t> spirvCache_;                 ///< Caché del bytecode SPIR-V generado durante la compilación.

  private:
    VKDevice* vk_device_; ///< Puntero al dispositivo Vulkan lógico propietario.
    std::string source_;    ///< Código fuente del shader almacenado temporalmente antes de la compilación.
  };

} // namespace mam