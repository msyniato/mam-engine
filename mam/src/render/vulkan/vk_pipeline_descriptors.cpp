#include "render/vulkan/vk_pipeline.hpp"
#include "render/vulkan/vk_device.hpp"
#include "render/vulkan/vk_texture.hpp"

namespace mam {


  void VKPipeline::createDescriptorSetLayout() {
    std::array<vk::DescriptorSetLayoutBinding, 5> bindings{};

    bindings[0].binding = 0;
    bindings[0].descriptorType = vk::DescriptorType::eUniformBuffer;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags =
      vk::ShaderStageFlagBits::eVertex |
      vk::ShaderStageFlagBits::eFragment;

    for (uint32_t i = 1; i <= 4; ++i) {
      bindings[i].binding = i;
      bindings[i].descriptorType = vk::DescriptorType::eCombinedImageSampler;
      bindings[i].descriptorCount = 1;
      bindings[i].stageFlags = vk::ShaderStageFlagBits::eFragment;
    }

    vk::DescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vk_device_->device->createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorSetLayout) != vk::Result::eSuccess) {
      throw std::runtime_error("failed to create descriptor set layout!");
    }

    // --- set=1: LightBuffer SSBO (binding 0) ---
    vk::DescriptorSetLayoutBinding ssboBinding{};
    ssboBinding.binding = 0;
    ssboBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
    ssboBinding.descriptorCount = 1;
    ssboBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

    vk::DescriptorSetLayoutCreateInfo ssboLayoutInfo{};
    ssboLayoutInfo.bindingCount = 1;
    ssboLayoutInfo.pBindings = &ssboBinding;

    if (vk_device_->device->createDescriptorSetLayout(
      &ssboLayoutInfo, nullptr, &descriptorSetLayout1) != vk::Result::eSuccess) {
      throw std::runtime_error("failed to create descriptor set layout (set=1)!");
    }

  }

  void VKPipeline::createDescriptorPool() {
    static constexpr u32 kMaxMaterialDescriptorSets = 256;

    std::array<vk::DescriptorPoolSize, 3> poolSizes{};

    poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
    poolSizes[0].descriptorCount = kMaxMaterialDescriptorSets;

    poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
    poolSizes[1].descriptorCount = kMaxMaterialDescriptorSets * 4;

    poolSizes[2].type = vk::DescriptorType::eStorageBuffer;
    poolSizes[2].descriptorCount = 1;

    vk::DescriptorPoolCreateInfo poolInfo{};
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = kMaxMaterialDescriptorSets + 1;

    if (vk_device_->device->createDescriptorPool(
      &poolInfo, nullptr, &descriptorPool) != vk::Result::eSuccess) {
      throw std::runtime_error("failed to create descriptor pool!");
    }
  }

  void VKPipeline::createDescriptorSets() {
    descriptorSet.resize(2);
    descriptorSet[0] = nullptr;
    descriptorSet[1] = nullptr;

    vk::DescriptorSetLayout lightLayout = descriptorSetLayout1;

    vk::DescriptorSetAllocateInfo allocInfo{};
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &lightLayout;

    if (vk_device_->device->allocateDescriptorSets(
      &allocInfo, &descriptorSet[1]) != vk::Result::eSuccess) {
      throw std::runtime_error("failed to allocate light descriptor set!");
    }

    createDummyTexture();
  }

  VKMaterialDescriptor* VKPipeline::getOrCreateMaterialDescriptorSet(const TextureSetKey& textures)
  {
    auto it = materialDescriptorSets_.find(textures);
    if (it != materialDescriptorSets_.end()) {
      return &it->second;
    }

    VKMaterialDescriptor descriptor = allocateMaterialDescriptorSet(textures);
    auto [insertedIt, inserted] = materialDescriptorSets_.emplace(textures, std::move(descriptor));
    return &insertedIt->second;
  }

  VKMaterialDescriptor VKPipeline::allocateMaterialDescriptorSet(const TextureSetKey& textures)
  {
    VKMaterialDescriptor descriptor;
    descriptor.cpuData.resize(sizeof(UniformBufferObject));

    vk_device_->createBuffer(
      sizeof(UniformBufferObject),
      vk::BufferUsageFlagBits::eUniformBuffer,
      vk::MemoryPropertyFlagBits::eHostVisible |
      vk::MemoryPropertyFlagBits::eHostCoherent,
      descriptor.uniformBuffer,
      descriptor.uniformMemory
    );

    vk::DescriptorSetLayout materialLayout = descriptorSetLayout;

    vk::DescriptorSetAllocateInfo allocInfo{};
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &materialLayout;

    if (vk_device_->device->allocateDescriptorSets(
      &allocInfo, &descriptor.descriptorSet) != vk::Result::eSuccess) {
      throw std::runtime_error("failed to allocate material descriptor set!");
    }

    vk::DescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = descriptor.uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    std::array<vk::DescriptorImageInfo, kMaxMaterialTextures> imageInfos{};

    std::vector<vk::WriteDescriptorSet> writes;
    writes.reserve(1 + kMaxMaterialTextures);

    vk::WriteDescriptorSet uboWrite{};
    uboWrite.dstSet = descriptor.descriptorSet;
    uboWrite.dstBinding = 0;
    uboWrite.dstArrayElement = 0;
    uboWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
    uboWrite.descriptorCount = 1;
    uboWrite.pBufferInfo = &bufferInfo;
    writes.push_back(uboWrite);

    for (u32 i = 0; i < kMaxMaterialTextures; ++i) {
      const Texture* tex = textures[i];

      const VKTexture* vkTex = tex
        ? static_cast<const VKTexture*>(tex)
        : dummyTexture_.get();

      imageInfos[i].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
      imageInfos[i].imageView = vkTex->imageView();
      imageInfos[i].sampler = vkTex->sampler();

      vk::WriteDescriptorSet texWrite{};
      texWrite.dstSet = descriptor.descriptorSet;
      texWrite.dstBinding = i + 1;
      texWrite.dstArrayElement = 0;
      texWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
      texWrite.descriptorCount = 1;
      texWrite.pImageInfo = &imageInfos[i];

      writes.push_back(texWrite);
    }

    vk_device_->device->updateDescriptorSets(
      static_cast<uint32_t>(writes.size()),
      writes.data(), 0, nullptr
    );

    return descriptor;
  }

  void VKPipeline::uploadMaterialDescriptorUniforms(VKMaterialDescriptor& descriptor)
  {
    if (!descriptor.uniformMemory || descriptor.cpuData.empty()) return;

    void* gpuPtr = vk_device_->device->mapMemory(
      descriptor.uniformMemory, 0, sizeof(UniformBufferObject));
    std::memcpy(gpuPtr, descriptor.cpuData.data(), sizeof(UniformBufferObject));
    vk_device_->device->unmapMemory(descriptor.uniformMemory);
  }

  void VKPipeline::bindForMaterial(vk::DescriptorSet materialDescriptorSet)
  {
    vk_device_->getCommandBuffer().bindPipeline(
      vk::PipelineBindPoint::eGraphics, graphicsPipeline
    );

    std::array<vk::DescriptorSet, 2> sets = {
      materialDescriptorSet, descriptorSet[1] };

    vk_device_->getCommandBuffer().bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics, pipelineLayout,
      0, static_cast<uint32_t>(sets.size()),
      sets.data(), 0, nullptr
    );
  }

  void VKPipeline::createDummyTexture() {
    Texture::TextureSettings ts;
    ts.format = Format::RGBA8;
    ts.minFilter = Filter::NEAREST;
    ts.magFilter = Filter::NEAREST;
    ts.wrap = Wrap::CLAMP_TO_EDGE;

    dummyTexture_ = std::make_unique<VKTexture>(vk_device_, 1, 1, ts);

    uint32_t white = 0xFFFFFFFF;
    dummyTexture_->setData(&white, sizeof(uint32_t));
  }


  void VKPipeline::bindTexture(const Texture* tex, u32 set, u32 binding) {
    // Deprecated in Vulkan path.
    // Textures are now bound through per-material descriptor sets.
  }

  void VKPipeline::bindLightBuffer(vk::Buffer buffer, vk::DeviceSize size)
  {
    if (!buffer || size == 0) return;

    if (boundLightBuffer_ == buffer && boundLightBufferSize_ == size) return;

    boundLightBuffer_ = buffer;
    boundLightBufferSize_ = size;

    vk::DescriptorBufferInfo info{};
    info.buffer = buffer;
    info.offset = 0;
    info.range = size;

    vk::WriteDescriptorSet write{};
    write.dstSet = descriptorSet[1];
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = vk::DescriptorType::eStorageBuffer;
    write.descriptorCount = 1;
    write.pBufferInfo = &info;

    vk_device_->device->updateDescriptorSets(1, &write, 0, nullptr);
  }



}
