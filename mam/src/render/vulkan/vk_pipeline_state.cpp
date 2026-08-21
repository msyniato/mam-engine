#include "render/vulkan/vk_pipeline.hpp"
#include "render/vulkan/vk_device.hpp"

namespace mam {

  void VKPipeline::initStructs() {
    addAttribute(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position));
    addAttribute(0, 1, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal));
    addAttribute(0, 2, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord));
    addAttribute(0, 3, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, tangent));

    setBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex);
    setInputAssembly();
    setViewport((float)vk_device_->swapChainExtent.width,
      (float)vk_device_->swapChainExtent.height);
    setScissor();
    setRasterizer();

    const u32 colorAttachmentCount = vk_device_
      ? vk_device_->getCurrentColorAttachmentCount() : 1;

    setColorAttachmentCount(colorAttachmentCount, false);
    setColorBlending();
  }

  void VKPipeline::addAttribute(u32 binding, u32 location, vk::Format format, u32 offset) {
    vk::VertexInputAttributeDescription attdesc;
    attdesc.binding = binding;
    attdesc.location = location;
    attdesc.format = format;
    attdesc.offset = offset;
    description.inputAttributes.push_back(attdesc);
  }

  void VKPipeline::setBindingDescription(u32 binding, u32 stride, vk::VertexInputRate inputRate) {
    description.inputBinding.binding = binding;
    description.inputBinding.stride = stride;
    description.inputBinding.inputRate = inputRate;
  }

  void VKPipeline::setVertexInputInfo() {
    description.vertexInputInfo.vertexBindingDescriptionCount = 1;
    description.vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(description.inputAttributes.size());
    description.vertexInputInfo.pVertexBindingDescriptions = &description.inputBinding;
    description.vertexInputInfo.pVertexAttributeDescriptions = description.inputAttributes.data();
  }

  void VKPipeline::setInputAssembly(vk::PrimitiveTopology drawMode) {
    description.inputAssembly.topology = drawMode;
    description.inputAssembly.primitiveRestartEnable = VK_FALSE;
  }

  void VKPipeline::setViewport(float width, float height, float depth) {
    description.viewport.x = 0.0f;
    description.viewport.y = 0.0f;
    description.viewport.width = width;
    description.viewport.height = height;
    description.viewport.minDepth = 0.0f;
    description.viewport.maxDepth = depth;
  }

  void VKPipeline::setScissor(s32 offsetX, s32 offsetY) {
    description.scissor.offset = VkOffset2D{ offsetX, offsetY };
    description.scissor.extent = vk_device_->swapChainExtent;
  }

  void VKPipeline::setViewportState() {
    description.viewportState = vk::PipelineViewportStateCreateInfo{};

    description.viewportState.viewportCount = 1;
    description.viewportState.pViewports = nullptr;

    description.viewportState.scissorCount = 1;
    description.viewportState.pScissors = nullptr;
  }

  void VKPipeline::setRasterizer(bool depthClampEnable, bool DiscardEnable,
    vk::PolygonMode mode, vk::CullModeFlagBits cullmode,
    vk::FrontFace frontface, bool depthBiasEnable) {

    if (depthClampEnable) {
      description.rasterizer.depthClampEnable = VK_TRUE;
    }
    else {
      description.rasterizer.depthClampEnable = VK_FALSE;
    }
    if (DiscardEnable) {
      description.rasterizer.rasterizerDiscardEnable = VK_TRUE;
    }
    else {
      description.rasterizer.rasterizerDiscardEnable = VK_FALSE;
    }
    description.rasterizer.polygonMode = mode;
    description.rasterizer.lineWidth = 1.0f;
    description.rasterizer.cullMode = cullmode;
    description.rasterizer.frontFace = frontface;

    if (depthBiasEnable) {
      description.rasterizer.depthBiasEnable = VK_TRUE;
    }
    else {
      description.rasterizer.depthBiasEnable = VK_FALSE;
    }
  }

  void VKPipeline::setMultisampling() {
    description.multisampling.sampleShadingEnable = VK_FALSE;
    description.multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
  }

  void VKPipeline::setColorBlendAttachment(bool blendEnable) {
    description.colorBlendAttachment.colorWriteMask =
      vk::ColorComponentFlagBits::eR |
      vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB |
      vk::ColorComponentFlagBits::eA;

    description.colorBlendAttachment.blendEnable = blendEnable ? VK_TRUE : VK_FALSE;
  }

  void VKPipeline::setColorAttachmentCount(u32 count, bool blendEnable) {
    count = std::max<u32>(1, count);

    description.colorBlendAttachments.clear();
    description.colorBlendAttachments.resize(count);

    for (auto& attachment : description.colorBlendAttachments) {
      attachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR |
        vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB |
        vk::ColorComponentFlagBits::eA;

      attachment.blendEnable = blendEnable ? VK_TRUE : VK_FALSE;
      attachment.srcColorBlendFactor = vk::BlendFactor::eOne;
      attachment.dstColorBlendFactor = vk::BlendFactor::eZero;
      attachment.colorBlendOp = vk::BlendOp::eAdd;
      attachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
      attachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
      attachment.alphaBlendOp = vk::BlendOp::eAdd;
    }

    description.colorBlending.attachmentCount =
      static_cast<u32>(description.colorBlendAttachments.size());

    description.colorBlending.pAttachments =
      description.colorBlendAttachments.data();
  }

  void VKPipeline::setColorBlending(bool logicOpEnable, vk::LogicOp logicOp) {
    description.colorBlending.logicOpEnable = logicOpEnable ? VK_TRUE : VK_FALSE;
    description.colorBlending.logicOp = logicOp;

    description.colorBlending.blendConstants[0] = 0.0f;
    description.colorBlending.blendConstants[1] = 0.0f;
    description.colorBlending.blendConstants[2] = 0.0f;
    description.colorBlending.blendConstants[3] = 0.0f;

    if (description.colorBlendAttachments.empty()) {
      setColorAttachmentCount(1, false);
    }
  }


}