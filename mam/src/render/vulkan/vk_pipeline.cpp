
#include "render/vulkan/vk_pipeline.hpp"
#include "render/vulkan/vk_context.hpp"
#include "render/vulkan/vk_device.hpp"
#include "render/vulkan/vk_shader.hpp"
#include "render/vulkan/vk_texture.hpp"

const int MAX_FRAMES_IN_FLIGHT = 2;

  static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
      throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
  }


namespace mam {

  uint32_t VKPipeline::FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties memProperties = 
      vk_device_->physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
      if ((typeFilter & (1 << i)) &&
        (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
        return i;
      }
    }

    throw std::runtime_error("failed to find suitable memory type!");
  }

  VKPipeline::VKPipeline(VKDevice* device) {
    vk_device_ = device;
  }

  VKPipeline::~VKPipeline() {
    destroy();
  }

  void VKPipeline::addShader(Shader* shader) {
    if (!shader) return;

    VKShader* vkShader = (VKShader*)shader;
    if (vkShader->type() == mam::Shader::Vertex) {
      description.shaderStages[0] = vkShader->shaderStage_;
      description.shaderModule[0] = vkShader->shaderModule_;
      description.spirvCache_[0] = vkShader->spirvCache_;
    }
    else if (vkShader->type() == mam::Shader::Fragment) {
      description.shaderStages[1] = vkShader->shaderStage_;
      description.shaderModule[1] = vkShader->shaderModule_;
      description.spirvCache_[1] = vkShader->spirvCache_;
    }
  }

  std::optional<std::string> VKPipeline::create() {
    description.inputAttributes.clear();

    createDescriptorSetLayout();

    initStructs();

    setVertexInputInfo();
    setViewportState();
    setMultisampling();

    renderPass = vk_device_->getCurrentRenderPass();

    if (!renderPass) {
      throw std::runtime_error(
        "VKPipeline::create() failed: no current render pass set. "
        "Create the VKFramebuffer before creating Vulkan materials/pipelines."
      );
    }

    vk::PipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = vk::CompareOp::eLess;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    std::array<vk::DescriptorSetLayout, 2> setLayouts = {
      descriptorSetLayout,
      descriptorSetLayout1
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.setLayoutCount = 2;
    pipelineLayoutInfo.pSetLayouts = setLayouts.data();


    if (!description.shaderStages[0].module || !description.shaderStages[1].module) {
      return "VKPipeline::create() failed: missing vertex or fragment shader module";
    }

    try {
      pipelineLayout = vk_device_->device->createPipelineLayout(pipelineLayoutInfo);
    }
    catch (vk::SystemError err) {
      throw std::runtime_error("failed to create pipeline layout!");
    }

    std::array<vk::DynamicState, 2> dynamicStates = {
      vk::DynamicState::eViewport,
      vk::DynamicState::eScissor
    };
    vk::PipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    vk::GraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = description.shaderStages;
    pipelineInfo.pVertexInputState = &description.vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &description.inputAssembly;
    pipelineInfo.pViewportState = &description.viewportState;
    pipelineInfo.pRasterizationState = &description.rasterizer;
    pipelineInfo.pMultisampleState = &description.multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &description.colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = nullptr;

    try {
      graphicsPipeline = vk_device_->device->createGraphicsPipeline(nullptr, pipelineInfo).value;
    }
    catch (vk::SystemError err) {
      throw std::runtime_error("failed to create graphics pipeline!");
    }

    createUniformBuffer();

    createDescriptorPool();
    createDescriptorSets();
    return std::nullopt;
  }

  void VKPipeline::recreatePipeline() {
    vk_device_->device->waitIdle();
    if (graphicsPipeline) {
      vk_device_->device->destroyPipeline(graphicsPipeline);
      graphicsPipeline = nullptr;
    }
    if (pipelineLayout) {
      vk_device_->device->destroyPipelineLayout(pipelineLayout);
      pipelineLayout = nullptr;
    }

    for (auto mod : ownedModules_) {
      if (mod) vk_device_->device->destroyShaderModule(mod);
    }
    ownedModules_.clear();

    for (int i = 0; i < 2; ++i) {
      if (description.spirvCache_[i].empty()) continue;

      vk::ShaderModuleCreateInfo ci{};
      ci.codeSize = description.spirvCache_[i].size() * sizeof(uint32_t);
      ci.pCode = description.spirvCache_[i].data();

      vk::ShaderModule newModule = vk_device_->device->createShaderModule(ci);
      description.shaderStages[i].module = newModule;
      description.shaderModule[i] = newModule;
      ownedModules_.push_back(newModule); 
    }

    description.inputAttributes.clear();

    renderPass = vk_device_->getCurrentRenderPass();
    assert(renderPass && "recreatePipeline: no renderPass");

    initStructs();
    setVertexInputInfo();
    setViewportState();
    setMultisampling();

    vk::PipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = vk::CompareOp::eLess;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    std::array<vk::DescriptorSetLayout, 2> setLayouts = {
      descriptorSetLayout,
      descriptorSetLayout1
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.setLayoutCount = 2;
    pipelineLayoutInfo.pSetLayouts = setLayouts.data();
    pipelineLayout = vk_device_->device->createPipelineLayout(pipelineLayoutInfo);

    std::array<vk::DynamicState, 2> dynamicStates = {
        vk::DynamicState::eViewport, vk::DynamicState::eScissor
    };
    vk::PipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    vk::GraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = description.shaderStages;
    pipelineInfo.pVertexInputState = &description.vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &description.inputAssembly;
    pipelineInfo.pViewportState = &description.viewportState;
    pipelineInfo.pRasterizationState = &description.rasterizer;
    pipelineInfo.pMultisampleState = &description.multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &description.colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    graphicsPipeline = vk_device_->device->createGraphicsPipeline(
      nullptr, pipelineInfo).value;
  }

  void VKPipeline::use() {
    TextureSetKey emptyTextures{};
    VKMaterialDescriptor* descriptor = getOrCreateMaterialDescriptorSet(emptyTextures);
    if (!descriptor) return;

    descriptor->cpuData = cpuUniformData();
    uploadMaterialDescriptorUniforms(*descriptor);
    bindForMaterial(descriptor->descriptorSet);
  }

  void VKPipeline::destroy() {
    if (!vk_device_ || !vk_device_->device) return;

    if (graphicsPipeline) {
      vk_device_->device->destroyPipeline(graphicsPipeline);
      graphicsPipeline = nullptr;
    }
    if (pipelineLayout) {
      vk_device_->device->destroyPipelineLayout(pipelineLayout);
      pipelineLayout = nullptr;
    }
    renderPass = nullptr;

    for (auto mod : ownedModules_) {
      if (mod) vk_device_->device->destroyShaderModule(mod);
    }
    ownedModules_.clear();

    for (auto& [key, desc] : materialDescriptorSets_) {
      if (desc.uniformBuffer) {
        vk_device_->device->destroyBuffer(desc.uniformBuffer);
        desc.uniformBuffer = nullptr;
      }
      if (desc.uniformMemory) {
        vk_device_->device->freeMemory(desc.uniformMemory);
        desc.uniformMemory = nullptr;
      }
      desc.cpuData.clear();
    }
    materialDescriptorSets_.clear();

    boundLightBuffer_ = nullptr;
    boundLightBufferSize_ = 0;

    if (descriptorPool) {
      vk_device_->device->destroyDescriptorPool(descriptorPool);
      descriptorPool = nullptr;
    }
    descriptorSet.clear();

    if (descriptorSetLayout) {
      vk_device_->device->destroyDescriptorSetLayout(descriptorSetLayout);
      descriptorSetLayout = nullptr;
    }

    if (descriptorSetLayout1) {
      vk_device_->device->destroyDescriptorSetLayout(descriptorSetLayout1);
      descriptorSetLayout1 = nullptr;
    }

    mappedUniforms_.clear();
    uniforms.clear();
    dummyTexture_.reset();
  }

  u32 VKPipeline::id() const {
    return 0;
  }

}