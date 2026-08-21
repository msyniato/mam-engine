#pragma once

#include "render/api/buffer.hpp"
#include <vulkan/vulkan.hpp>

namespace mam {

  class VKDevice;
  class VKPipeline;

  /**
   * @brief Implementación Vulkan de un buffer de vértices.
   *
   * Gestiona un `vk::Buffer` de Vulkan destinado a almacenar datos de vértices
   * junto con la memoria de dispositivo (`vk::DeviceMemory`) asociada.
   * Permite subir datos desde la CPU y enlazarlo al pipeline de renderizado.
   *
   * @note Esta clase no es copiable ni movible.
   * @see VKDevice, VKIndexBuffer
   */
  class VKVertexBuffer : public VertexBuffer {
  public:
    /**
     * @brief Constructor.
     * @param device Puntero al dispositivo Vulkan lógico propietario de este buffer.
     */
    VKVertexBuffer(VKDevice* device);

    /**
     * @brief Destructor.
     *
     * Libera el buffer de Vulkan y la memoria de dispositivo asociada.
     */
    ~VKVertexBuffer() override;

    VKVertexBuffer(const VKVertexBuffer&) = delete;
    VKVertexBuffer& operator=(const VKVertexBuffer&) = delete;
    VKVertexBuffer(VKVertexBuffer&&) noexcept = delete;
    VKVertexBuffer& operator=(VKVertexBuffer&&) noexcept = delete;

    /**
     * @brief Enlaza el buffer de vértices para su uso en el renderizado.
     *
     * Registra el buffer en el command buffer activo como fuente de vértices.
     */
    void bind() override;

    /**
     * @brief Sube datos al buffer de vértices en la memoria del dispositivo.
     *
     * Copia los datos indicados desde la CPU al buffer de Vulkan, comenzando
     * en el desplazamiento especificado.
     *
     * @param data   Puntero a los datos de vértices a subir.
     * @param size   Tamaño de los datos en bytes.
     * @param offset Desplazamiento en bytes dentro del buffer donde comenzar la escritura (por defecto 0).
     */
    void uploadData(const void* data, unsigned int size, unsigned int offset = 0) override;

    /**
     * @brief Libera el buffer de Vulkan y su memoria de dispositivo.
     *
     * Invalida los handles @ref vertexBuffer y @ref vertexBufferMemory.
     */
    void release() override;

    vk::Buffer       vertexBuffer;        ///< Handle al buffer de vértices de Vulkan.
    vk::DeviceMemory vertexBufferMemory;  ///< Memoria de dispositivo asociada al buffer de vértices.

  protected:
    VKDevice* vk_device_; ///< Puntero al dispositivo Vulkan lógico propietario.
  };


  /**
   * @brief Implementación Vulkan de un buffer de índices.
   *
   * Gestiona un `vk::Buffer` de Vulkan destinado a almacenar índices de vértices
   * junto con la memoria de dispositivo asociada. Se usa junto a @ref VKVertexBuffer
   * para el renderizado indexado.
   *
   * @note Esta clase no es copiable ni movible.
   * @see VKDevice, VKVertexBuffer
   */
  class VKIndexBuffer : public IndexBuffer {
  public:
    /**
     * @brief Constructor.
     * @param device Puntero al dispositivo Vulkan lógico propietario de este buffer.
     */
    VKIndexBuffer(VKDevice* device);

    /**
     * @brief Destructor.
     *
     * Libera el buffer de Vulkan y la memoria de dispositivo asociada.
     */
    ~VKIndexBuffer() override;

    VKIndexBuffer(const VKIndexBuffer&) = delete;
    VKIndexBuffer& operator=(const VKIndexBuffer&) = delete;
    VKIndexBuffer(VKIndexBuffer&&) noexcept = delete;
    VKIndexBuffer& operator=(VKIndexBuffer&&) noexcept = delete;

    /**
     * @brief Enlaza el buffer de índices para su uso en el renderizado.
     *
     * Registra el buffer en el command buffer activo como fuente de índices.
     */
    void bind() override;

    /**
     * @brief Sube datos al buffer de índices en la memoria del dispositivo.
     *
     * Copia los datos indicados desde la CPU al buffer de Vulkan, comenzando
     * en el desplazamiento especificado.
     *
     * @param data   Puntero a los datos de índices a subir.
     * @param size   Tamaño de los datos en bytes.
     * @param offset Desplazamiento en bytes dentro del buffer donde comenzar la escritura (por defecto 0).
     */
    void uploadData(const void* data, unsigned int size, unsigned int offset = 0) override;

    /**
     * @brief Libera el buffer de Vulkan y su memoria de dispositivo.
     *
     * Invalida los handles @ref indexBuffer y @ref indexBufferMemory.
     */
    void release() override;

    vk::Buffer       indexBuffer;        ///< Handle al buffer de índices de Vulkan.
    vk::DeviceMemory indexBufferMemory;  ///< Memoria de dispositivo asociada al buffer de índices.

  protected:
    VKDevice* vk_device_; ///< Puntero al dispositivo Vulkan lógico propietario.
  };


  /**
   * @brief Implementación Vulkan de un Shader Storage Buffer Object (SSBO).
   *
   * Gestiona un buffer de almacenamiento de shader (`vk::Buffer`) que puede ser
   * leído y escrito directamente por los shaders. Permite asociarse a un
   * @ref VKPipeline para actualizar su descriptor set correspondiente.
   *
   * @note Esta clase no es copiable ni movible.
   * @see VKDevice, VKPipeline
   */
  class VKStorageBuffer : public ShaderStorageBuffer {
  public:
    /**
     * @brief Constructor.
     * @param device  Puntero al dispositivo Vulkan lógico propietario.
     * @param binding Índice de binding del descriptor al que se asociará este SSBO en el shader.
     */
    VKStorageBuffer(VKDevice* device, u32 binding);

    /**
     * @brief Destructor.
     *
     * Libera el buffer SSBO y su memoria de dispositivo si han sido creados.
     */
    ~VKStorageBuffer() override;

    VKStorageBuffer(const VKStorageBuffer&) = delete;
    VKStorageBuffer& operator=(const VKStorageBuffer&) = delete;
    VKStorageBuffer(VKStorageBuffer&&) noexcept = delete;
    VKStorageBuffer& operator=(VKStorageBuffer&&) noexcept = delete;

    /**
     * @brief Enlaza el SSBO para su uso en el pipeline activo.
     */
    void bind() override;

    /**
     * @brief Desenlaza el SSBO del pipeline activo.
     */
    void unBind() override;

    /**
     * @brief Sube datos al buffer SSBO en la memoria del dispositivo.
     *
     * @param data   Puntero a los datos a subir.
     * @param size   Tamaño de los datos en bytes (tipo @c u16).
     * @param offset Desplazamiento en bytes dentro del buffer (tipo @c u8).
     */
    void uploadData(const void* data, u16 size, u8 offset) override;

    /**
     * @brief Libera el buffer SSBO y su memoria de dispositivo.
     */
    void release() override;

    /**
     * @brief Asocia el SSBO a un pipeline Vulkan actualizando su descriptor set.
     *
     * Llama internamente a `pipeline->bindLightBuffer()`. Debe invocarse
     * después de @ref uploadData(), típicamente desde `ForwardPass::uploadLights()`.
     *
     * @param pipeline Puntero al pipeline Vulkan al que se enlazará el buffer.
     */
    void bindToPipeline(VKPipeline* pipeline);

    /**
     * @brief Devuelve el handle nativo al buffer de Vulkan.
     * @return Handle `vk::Buffer` del SSBO.
     */
    vk::Buffer     nativeBuffer() const { return ssboBuffer_; }

    /**
     * @brief Devuelve el tamaño actual del buffer en bytes.
     * @return Tamaño del buffer como `vk::DeviceSize`.
     */
    vk::DeviceSize nativeSize()   const { return currentSize_; }

    /**
     * @brief Indica si el buffer está listo para ser usado.
     * @return @c true si el buffer ha sido creado y es válido.
     */
    bool           isReady()      const { return bufferReady_; }

  private:
    VKDevice* vk_device_ = nullptr; ///< Puntero al dispositivo Vulkan lógico propietario.
    u32              binding_ = 0;       ///< Índice de binding del descriptor en el shader.

    vk::Buffer       ssboBuffer_ = nullptr; ///< Handle al buffer SSBO de Vulkan.
    vk::DeviceMemory ssboMemory_ = nullptr; ///< Memoria de dispositivo asociada al SSBO.
    vk::DeviceSize   currentSize_ = 0;       ///< Tamaño actual del buffer en bytes.
    bool             bufferReady_ = false;   ///< Indica si el buffer ha sido creado correctamente.
  };

} // namespace mam