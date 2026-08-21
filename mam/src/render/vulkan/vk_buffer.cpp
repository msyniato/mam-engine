
#include "render/vulkan/vk_buffer.hpp"
#include "render/vulkan/vk_context.hpp"
#include "render/vulkan/vk_device.hpp"
#include "render/vulkan/vk_pipeline.hpp"

namespace mam {

#pragma region VERTEX_BUFFER

  VKVertexBuffer::VKVertexBuffer(VKDevice* device) {
    vk_device_ = device;
  }

  VKVertexBuffer::~VKVertexBuffer(){
    release();
  }

  void VKVertexBuffer::bind() {
    vk::Buffer vertexBuffers[] = { vertexBuffer };
    vk::DeviceSize offsets[] = { 0 };
    vk_device_->getCommandBuffer().bindVertexBuffers(0, 1, vertexBuffers, offsets);
  }

  void VKVertexBuffer::uploadData(const void* data, unsigned int size, unsigned int offset) {
    release();
    vk::DeviceSize bufferSize = size;

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;            
    vk_device_->createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                             vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, //la cpu puede verlo
                             stagingBuffer, stagingBufferMemory);

    vk::UniqueDevice& device = vk_device_->device;

    void* mapData = device->mapMemory(stagingBufferMemory, 0, bufferSize);
    memcpy(mapData, data, (size_t)bufferSize);
    device->unmapMemory(stagingBufferMemory);

    vk_device_->createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                             vk::MemoryPropertyFlagBits::eDeviceLocal, vertexBuffer, vertexBufferMemory);

    vk_device_->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    device->destroyBuffer(stagingBuffer);
    device->freeMemory(stagingBufferMemory);
  }

  void VKVertexBuffer::release() {
    auto& dev = vk_device_->device;
    if (vertexBuffer) {
      dev->destroyBuffer(vertexBuffer);
      vertexBuffer = nullptr;
    }
    if (vertexBufferMemory) {
      dev->freeMemory(vertexBufferMemory);
      vertexBufferMemory = nullptr;
    }
  }

#pragma endregion

#pragma region INDEX_BUFFER
  VKIndexBuffer::VKIndexBuffer(VKDevice* device) {
    vk_device_ = device;
    id_ = 0;
  }

  VKIndexBuffer::~VKIndexBuffer() {
    release();
  }

  void VKIndexBuffer::bind() {
    vk::Buffer indexBuffers[] = { indexBuffer };
    vk::DeviceSize offsets[] = { 0 };

    vk_device_->getCommandBuffer().bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);
  }

  void VKIndexBuffer::uploadData(const void* data, unsigned int size, unsigned int offset) {
    release();
    vk::DeviceSize bufferSize = size;
    size_ = size;
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    vk_device_->createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
      stagingBuffer, stagingBufferMemory);

    vk::UniqueDevice& device = vk_device_->device;

    void* mapData = device->mapMemory(stagingBufferMemory, 0, bufferSize);
    memcpy(mapData, data, (size_t)bufferSize);
    device->unmapMemory(stagingBufferMemory);

    vk_device_->createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
      vk::MemoryPropertyFlagBits::eDeviceLocal, indexBuffer, indexBufferMemory);

    vk_device_->copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    device->destroyBuffer(stagingBuffer);
    device->freeMemory(stagingBufferMemory);
  }

  void VKIndexBuffer::release() {
    auto& dev = vk_device_->device;
    if (indexBuffer) {
      dev->destroyBuffer(indexBuffer);
      indexBuffer = nullptr;
    }
    if (indexBufferMemory) {
      dev->freeMemory(indexBufferMemory);
      indexBufferMemory = nullptr;
    }
  }

#pragma endregion

#pragma region STORAGE_BUFFER

  VKStorageBuffer::VKStorageBuffer(VKDevice* device, u32 binding)
    : vk_device_(device), binding_(binding)
  {
  }

  VKStorageBuffer::~VKStorageBuffer() {
    release();
  }

  void VKStorageBuffer::bind() {

  }

  void VKStorageBuffer::unBind() {

  }

  void VKStorageBuffer::uploadData(const void* data, u16 size, u8 offset) {
    if (!data || size == 0) return;

    vk::DeviceSize bufferSize = static_cast<vk::DeviceSize>(size);

    if (!ssboBuffer_ || currentSize_ < bufferSize) {
      release();

      vk_device_->createBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent,
        ssboBuffer_,
        ssboMemory_
      );

      currentSize_ = bufferSize;
    }

    void* mapped = vk_device_->device->mapMemory(ssboMemory_, offset, bufferSize);
    std::memcpy(mapped, data, static_cast<size_t>(bufferSize));
    vk_device_->device->unmapMemory(ssboMemory_);

    bufferReady_ = true;
  }

  void VKStorageBuffer::release() {
    if (!vk_device_ || !vk_device_->device) return;

    if (ssboBuffer_) {
      vk_device_->device->destroyBuffer(ssboBuffer_);
      ssboBuffer_ = nullptr;
    }
    if (ssboMemory_) {
      vk_device_->device->freeMemory(ssboMemory_);
      ssboMemory_ = nullptr;
    }

    currentSize_ = 0;
    bufferReady_ = false;
  }

  void VKStorageBuffer::bindToPipeline(VKPipeline* pipeline) {
    if (!pipeline || !bufferReady_ || !ssboBuffer_) return;
    pipeline->bindLightBuffer(ssboBuffer_, currentSize_);
  }


#pragma endregion

}