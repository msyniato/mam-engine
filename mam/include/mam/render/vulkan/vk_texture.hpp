#pragma once

#include "render/api/texture.hpp"
#include <vulkan/vulkan.hpp>

namespace mam {

  class VKDevice;

  /**
   * @brief Implementación Vulkan de la interfaz @c Texture.
   *
   * Gestiona un objeto de textura respaldado por Vulkan que cubre todo el ciclo
   * de vida: creación de la imagen (`vk::Image`), asignación de memoria de
   * dispositivo, subida de datos de píxeles, configuración del muestreador
   * (`vk::Sampler`), generación de mipmaps y enlace a slots de descriptores.
   *
   * Soporta tres vías de construcción:
   *  - Sin datos iniciales (solo dispositivo).
   *  - Carga desde archivo en disco.
   *  - Creación con dimensiones y ajustes personalizados.
   *
   * @note Esta clase no es copiable ni movible.
   * @see VKDevice, TextureSettings
   */
  class VKTexture : public Texture {
  public:
    /**
     * @brief Constructor por defecto (sin imagen cargada).
     * @param device Puntero al dispositivo Vulkan lógico propietario.
     */
    VKTexture(VKDevice* device);

    /**
     * @brief Constructor que carga la textura desde un archivo.
     * @param device   Puntero al dispositivo Vulkan lógico propietario.
     * @param filepath Ruta al archivo de imagen.
     * @param relative Si es @c true, la ruta se interpreta relativa al directorio de trabajo del motor.
     */
    explicit VKTexture(VKDevice* device, const std::string& filepath, bool relative = true);

    /**
     * @brief Constructor que crea una textura vacía con dimensiones y ajustes dados.
     * @param device   Puntero al dispositivo Vulkan lógico propietario.
     * @param width    Anchura de la textura en píxeles.
     * @param height   Altura de la textura en píxeles.
     * @param settings Configuración de formato, filtros y wrapping (opcional; usa valores por defecto si se omite).
     */
    VKTexture(VKDevice* device, u32 width, u32 height, const TextureSettings& settings = {});

    /**
     * @brief Destructor.
     *
     * Destruye la imagen, la vista de imagen, el muestreador y libera la
     * memoria de dispositivo a través de @ref destroyResources().
     */
    ~VKTexture() override;

    VKTexture(const VKTexture&) = delete;
    VKTexture& operator=(const VKTexture&) = delete;
    VKTexture(VKTexture&&) noexcept = delete;
    VKTexture& operator=(VKTexture&&) noexcept = delete;

    /**
     * @brief Sube datos de píxeles a la imagen de la textura.
     *
     * Crea un buffer de staging, copia los datos, realiza la transición de
     * layout necesaria y transfiere el contenido a la imagen de Vulkan.
     *
     * @param data      Puntero a los datos de píxeles en bruto.
     * @param sizeBytes Tamaño de los datos en bytes.
     */
    void setData(const void* data, u32 sizeBytes) override;

    /**
     * @brief Sobrecarga de @ref setData() con parámetros adicionales (ignorados en Vulkan).
     *
     * Los dos últimos parámetros existen únicamente por compatibilidad con la
     * interfaz OpenGL y son ignorados en esta implementación.
     *
     * @param data      Puntero a los datos de píxeles en bruto.
     * @param sizeBytes Tamaño de los datos en bytes.
     */
    void setData(const void* data, u32 sizeBytes, u32 /*ignored – GL only*/, u32 /*ignored*/) override;

    /**
     * @brief Recarga la textura desde un nuevo archivo en disco.
     *
     * Destruye los recursos actuales y vuelve a crearlos a partir del archivo
     * especificado.
     *
     * @param filepath Ruta al nuevo archivo de imagen.
     * @param relative Si es @c true, la ruta se interpreta como relativa al directorio de trabajo.
     */
    void resetTexture(const std::string& filepath, bool relative = true) override;

    /**
     * @brief Establece el filtro de minificación del muestreador.
     *
     * Llama internamente a @ref rebuildSampler() para aplicar el cambio.
     *
     * @param f Modo de filtrado (@c Filter::Nearest, @c Filter::Linear, etc.).
     */
    void setMinFilter(Filter f) override;

    /**
     * @brief Establece el filtro de magnificación del muestreador.
     *
     * Llama internamente a @ref rebuildSampler() para aplicar el cambio.
     *
     * @param f Modo de filtrado.
     */
    void setMagFilter(Filter f) override;

    /**
     * @brief Establece el modo de wrapping para la coordenada S (U).
     * @param c Modo de wrapping (@c Wrap::Repeat, @c Wrap::ClampToEdge, etc.).
     */
    void setWrapS(Wrap c) override;

    /**
     * @brief Establece el modo de wrapping para la coordenada T (V).
     * @param c Modo de wrapping.
     */
    void setWrapT(Wrap c) override;

    /**
     * @brief Establece el modo de wrapping para la coordenada R (W).
     * @param c Modo de wrapping.
     */
    void setWrapR(Wrap c) override;

    /**
     * @brief Establece el mismo modo de wrapping para las tres coordenadas (S, T y R).
     * @param c Modo de wrapping aplicado uniformemente.
     */
    void setWrap(Wrap c) override;

    /**
     * @brief Genera la cadena de mipmaps para la textura actual.
     *
     * Calcula los niveles de mipmap necesarios y registra las transiciones de
     * layout y los blits en el command buffer temporal.
     */
    void generateMipmaps() override;

    /**
     * @brief Enlaza la textura a un slot de textura del pipeline activo.
     * @param slot Unidad de textura a la que se enlaza (por defecto 0).
     */
    void bind(u32 slot = 0) const override;

    /**
     * @brief Desenlaza la textura de un slot de textura del pipeline activo.
     * @param slot Unidad de textura de la que se desenlaza (por defecto 0).
     */
    void unBind(u32 slot = 0) const override;

    /**
     * @brief Devuelve la vista de imagen de Vulkan asociada a esta textura.
     * @return Handle `vk::ImageView`.
     */
    vk::ImageView   imageView()   const { return textureImageView_; }

    /**
     * @brief Devuelve el muestreador de Vulkan asociado a esta textura.
     * @return Handle `vk::Sampler`.
     */
    vk::Sampler     sampler()     const { return sampler_; }

    /**
     * @brief Devuelve el layout de imagen actual.
     * @return Estado de layout (`vk::ImageLayout`) de la imagen en este momento.
     */
    vk::ImageLayout imageLayout() const { return currentLayout_; }

    /**
     * @brief Construye y devuelve un `VkDescriptorImageInfo` listo para usar en un descriptor set.
     * @return Estructura `vk::DescriptorImageInfo` con la vista de imagen, el muestreador y el layout actuales.
     */
    vk::DescriptorImageInfo descriptorInfo() const;

  protected:

    /**
     * @brief Crea la imagen de Vulkan y asigna memoria de dispositivo.
     *
     * @param width      Anchura en píxeles.
     * @param height     Altura en píxeles.
     * @param mipLevels  Número de niveles de mipmap.
     * @param format     Formato de píxel de la imagen (`vk::Format`).
     * @param tiling     Modo de tiling de la imagen (`vk::ImageTiling`).
     * @param usage      Flags de uso de la imagen (`vk::ImageUsageFlags`).
     * @param properties Propiedades de memoria requeridas (`vk::MemoryPropertyFlags`).
     */
    void createImage(u32 width, u32 height, u32 mipLevels, vk::Format format,
      vk::ImageTiling tiling, vk::ImageUsageFlags usage,
      vk::MemoryPropertyFlags properties);

    /**
     * @brief Crea la vista de imagen (`vk::ImageView`) para la imagen creada.
     *
     * @param format      Formato de píxel de la imagen.
     * @param aspectFlags Flags de aspecto de imagen (color, profundidad, stencil…).
     * @param mipLevels   Número de niveles de mipmap accesibles desde la vista.
     */
    void createImageView(vk::Format format,
      vk::ImageAspectFlags aspectFlags,
      u32 mipLevels);

    /**
     * @brief Crea el muestreador (`vk::Sampler`) con los ajustes actuales.
     * @param mipLevels Número máximo de niveles de mipmap del muestreador.
     */
    void createSampler(u32 mipLevels);

    /**
     * @brief Destruye todos los recursos de Vulkan asociados a esta textura.
     *
     * Libera la imagen, la memoria, la vista de imagen y el muestreador.
     */
    void destroyResources();

    /**
     * @brief Copia datos desde un buffer de staging a la imagen de Vulkan.
     *
     * Gestiona automáticamente las transiciones de layout necesarias antes y
     * después de la operación de copia.
     *
     * @param buffer Handle al buffer de staging con los datos de origen.
     * @param width  Anchura en píxeles de la región a copiar.
     * @param height Altura en píxeles de la región a copiar.
     */
    void copyBufferToImage(vk::Buffer buffer, u32 width, u32 height);

    /**
     * @brief Registra una transición de layout de imagen en un command buffer temporal.
     *
     * @param oldLayout Layout de origen.
     * @param newLayout Layout de destino.
     * @param mipLevels Número de niveles de mipmap afectados por la transición (por defecto 1).
     */
    void transitionImageLayout(vk::ImageLayout oldLayout,
      vk::ImageLayout newLayout,
      u32 mipLevels = 1);

    /**
     * @brief Reconstruye el muestreador a partir de la configuración interna actual (`setts_`).
     *
     * Se invoca automáticamente tras cambios en los filtros o el modo de wrapping.
     */
    void rebuildSampler();

    /** @brief Convierte un @c Format del motor al equivalente `vk::Format`. */
    static vk::Format            toVKFormat(Format f);

    /** @brief Convierte un @c Filter del motor al equivalente `vk::Filter`. */
    static vk::Filter            toVKFilter(Filter f);

    /** @brief Convierte un @c Filter del motor al modo de mipmap `vk::SamplerMipmapMode`. */
    static vk::SamplerMipmapMode toVKMipmapMode(Filter f);

    /** @brief Convierte un @c Wrap del motor al equivalente `vk::SamplerAddressMode`. */
    static vk::SamplerAddressMode toVKWrap(Wrap c);

    /**
     * @brief Indica si el filtro especificado requiere la generación de mipmaps.
     * @param f Modo de filtrado a evaluar.
     * @return @c true si el filtro usa mipmaps (p. ej. `Filter::LinearMipmap`).
     */
    static bool wantsMipmaps(Filter f);

    /**
     * @brief Calcula el número de niveles de mipmap para una imagen de las dimensiones dadas.
     * @param w Anchura en píxeles.
     * @param h Altura en píxeles.
     * @return Número de niveles de mipmap: `floor(log2(max(w, h))) + 1`.
     */
    static u32  calcMipLevels(u32 w, u32 h);

    vk::Image        textureImage_;       ///< Handle a la imagen de Vulkan.
    vk::DeviceMemory textureImageMemory_; ///< Memoria de dispositivo asignada a la imagen.
    vk::ImageView    textureImageView_;   ///< Vista de imagen utilizada en los descriptores.
    vk::Sampler      sampler_;            ///< Muestreador de Vulkan con los ajustes de filtrado y wrapping.
    vk::ImageLayout  currentLayout_;      ///< Layout de imagen actual; se actualiza en cada transición.
    u32              mipLevels_ = 1;      ///< Número de niveles de mipmap generados para esta textura.

    VKDevice* vk_device_; ///< Puntero al dispositivo Vulkan lógico propietario.
  };

} // namespace mam