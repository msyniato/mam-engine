#include "render/vulkan/vk_framebuffer.hpp"
#include "render/vulkan/vk_device.hpp"
#include "render/vulkan/vk_context.hpp"
#include "render/vulkan/vk_texture.hpp"

namespace mam {

  vk::Format VKFramebuffer::toVKFormat(Format f) {
    switch (f) {
    case Format::RGBA8:           return vk::Format::eR8G8B8A8Unorm;
    case Format::RGBA16F:         return vk::Format::eR16G16B16A16Sfloat;
    case Format::RGBA32F:         return vk::Format::eR32G32B32A32Sfloat;
    case Format::RG32F:           return vk::Format::eR32G32Sfloat;
    case Format::DEPTH32F:        return vk::Format::eD32Sfloat;
    case Format::DEPTH24STENCIL8: return vk::Format::eD24UnormS8Uint;
    }
    return vk::Format::eR8G8B8A8Srgb;
  }

  vk::ImageLayout VKFramebuffer::finalLayoutFor(Format f) {
    switch (f) {
    case Format::DEPTH32F:
    case Format::DEPTH24STENCIL8: return vk::ImageLayout::eDepthStencilAttachmentOptimal;
    default:                      return vk::ImageLayout::eShaderReadOnlyOptimal;
    }
  }

  VKFramebuffer::VKFramebuffer(VKDevice* device, const FrameBufferSpec& spec) : FrameBuffer(spec) {
    vk_device_ = device;
    invalidate();
  }

  VKFramebuffer::~VKFramebuffer() {
    destroyResources();
  }

  void VKFramebuffer::bind() const {
    vk_device_->setCurrentRenderPass(renderPass_);
    vk_device_->setCurrentColorAttachmentCount(
      static_cast<u32>(spec_.colorFormats.size())
    );

    beginRenderPass(vk_device_->getCommandBuffer());
  }

  void VKFramebuffer::unbind() const {
    endRenderPass(vk_device_->getCommandBuffer());
  }

  void VKFramebuffer::resize(u32 width, u32 height) {
    if (width == 0 || height == 0) return;
    if (width == spec_.width && height == spec_.height) return;

    vk_device_->device->waitIdle();
    spec_.width = width;
    spec_.height = height;
    invalidate();

    vk_device_->setCurrentRenderPass(renderPass_);

    /*
    if (vk_device_->pipeline) {
      vk_device_->pipeline->setViewport(
        static_cast<float>(width),
        static_cast<float>(height)
      );
      vk_device_->pipeline->recreatePipeline();
    }

    for (auto& wp : vk_device_->registeredPipelines_) {
      if (auto sp = wp.lock()) {
        sp->setViewport(
          static_cast<float>(width), 
          static_cast<float>(height));
        sp->recreatePipeline();
      }
    }
    */

  }

  void VKFramebuffer::invalidate() {
    if (renderPass_ || framebuffer_)
      destroyResources();

    createRenderPass();
    createAttachments();
    createFramebuffer();
  }

  void VKFramebuffer::destroyResources() {
    if (framebuffer_) {
      vk_device_->device->destroyFramebuffer(framebuffer_);
      framebuffer_ = nullptr;
    }
    if (renderPass_) {
      vk_device_->device->destroyRenderPass(renderPass_);
      renderPass_ = nullptr;
    }

    vkColorAttachments_.clear();
    vkDepthAttachment_.reset();
    colorAttachments_.clear();
    depthAttachment_.reset();
  }

  void VKFramebuffer::createRenderPass() {
    std::vector<vk::AttachmentDescription> attachments;
    std::vector<vk::AttachmentReference> colorRefs;

    vk::AttachmentReference depthRef{};
    bool hasDepth = spec_.hasDepth;

    // Color attachments
    for (u32 i = 0; i < static_cast<u32>(spec_.colorFormats.size()); ++i) {
      vk::AttachmentDescription desc{};
      desc.format = toVKFormat(spec_.colorFormats[i]);
      desc.samples = vk::SampleCountFlagBits::e1;
      desc.loadOp = vk::AttachmentLoadOp::eClear;
      desc.storeOp = vk::AttachmentStoreOp::eStore;
      desc.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
      desc.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
      desc.initialLayout = vk::ImageLayout::eUndefined;
      desc.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

      attachments.push_back(desc);

      vk::AttachmentReference colorRef{};
      colorRef.attachment = i;
      colorRef.layout = vk::ImageLayout::eColorAttachmentOptimal;
      colorRefs.push_back(colorRef);
    }

    if (hasDepth) {
      vk::AttachmentDescription desc{};
      desc.format = toVKFormat(spec_.depthFormat);
      desc.samples = vk::SampleCountFlagBits::e1;
      desc.loadOp = vk::AttachmentLoadOp::eClear;
      desc.storeOp = vk::AttachmentStoreOp::eStore;
      desc.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
      desc.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
      desc.initialLayout = vk::ImageLayout::eUndefined;

      desc.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

      depthRef.attachment = static_cast<u32>(attachments.size());
      depthRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

      attachments.push_back(desc);
    }

    vk::SubpassDescription subpass{};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = static_cast<u32>(colorRefs.size());
    subpass.pColorAttachments = colorRefs.empty() ? nullptr : colorRefs.data();
    subpass.pDepthStencilAttachment = hasDepth ? &depthRef : nullptr;

    std::array<vk::SubpassDependency, 2> dependencies{};

    // External -> subpass
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eFragmentShader;
    dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependencies[0].srcAccessMask = vk::AccessFlagBits::eShaderRead;
    dependencies[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

    // Subpass -> external
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
    dependencies[1].srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    dependencies[1].dstAccessMask = vk::AccessFlagBits::eShaderRead;
    dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

    vk::RenderPassCreateInfo renderPassInfo{};
    renderPassInfo.attachmentCount = static_cast<u32>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<u32>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    renderPass_ = vk_device_->device->createRenderPass(renderPassInfo);
  }

  void VKFramebuffer::createAttachments() {
    vkColorAttachments_.resize(spec_.colorFormats.size());
    colorAttachments_.resize(spec_.colorFormats.size());

    for (u32 i = 0; i < static_cast<u32>(spec_.colorFormats.size()); ++i) {
      Texture::TextureSettings ts;
      ts.format = spec_.colorFormats[i];
      ts.minFilter = Filter::LINEAR;
      ts.magFilter = Filter::LINEAR;
      ts.wrap = Wrap::CLAMP_TO_EDGE;

      auto tex = std::make_shared<VKTexture>(
        vk_device_,
        spec_.width,
        spec_.height,
        ts
      );

      vkColorAttachments_[i] = tex;
      colorAttachments_[i] = tex;
    }

    if (spec_.hasDepth) {
      Texture::TextureSettings ts;
      ts.format = spec_.depthFormat;
      ts.minFilter = Filter::NEAREST;
      ts.magFilter = Filter::NEAREST;
      ts.wrap = Wrap::CLAMP_TO_EDGE;

      auto tex = std::make_shared<VKTexture>(
        vk_device_,
        spec_.width,
        spec_.height,
        ts
      );

      vkDepthAttachment_ = tex;
      depthAttachment_ = tex;
    }
  }

  void VKFramebuffer::createFramebuffer() {
    std::vector<vk::ImageView> views;
    views.reserve(vkColorAttachments_.size() + (spec_.hasDepth ? 1 : 0));

    for (auto& tex : vkColorAttachments_)
      views.push_back(tex->imageView());

    if (vkDepthAttachment_)
      views.push_back(vkDepthAttachment_->imageView());

    vk::FramebufferCreateInfo fbInfo{};
    fbInfo.renderPass = renderPass_;
    fbInfo.attachmentCount = static_cast<u32>(views.size());
    fbInfo.pAttachments = views.data();
    fbInfo.width = spec_.width;
    fbInfo.height = spec_.height;
    fbInfo.layers = 1;

    framebuffer_ = vk_device_->device->createFramebuffer(fbInfo);
  }

  void VKFramebuffer::beginRenderPass(vk::CommandBuffer cmd) const {
    std::vector<vk::ClearValue> clearValues;
    clearValues.reserve(vkColorAttachments_.size() + 1);

    for (size_t i = 0; i < vkColorAttachments_.size(); ++i) {
      vk::ClearValue cv{};
      cv.color = vk::ClearColorValue{ std::array<float,4>{0.0f, 0.0f, 0.0f, 1.0f} };
      clearValues.push_back(cv);
    }

    if (vkDepthAttachment_) {
      vk::ClearValue cv{};
      cv.depthStencil = vk::ClearDepthStencilValue{ 1.0f, 0 };
      clearValues.push_back(cv);
    }

    vk::RenderPassBeginInfo rpBegin{};
    rpBegin.renderPass = renderPass_;
    rpBegin.framebuffer = framebuffer_;
    rpBegin.renderArea.offset = vk::Offset2D{ 0, 0 };
    rpBegin.renderArea.extent = vk::Extent2D{ spec_.width, spec_.height };
    rpBegin.clearValueCount = static_cast<u32>(clearValues.size());
    rpBegin.pClearValues = clearValues.data();

    cmd.beginRenderPass(rpBegin, vk::SubpassContents::eInline);

    vk::Viewport viewport{ 0.0f, 0.0f,
                           static_cast<float>(spec_.width),
                           static_cast<float>(spec_.height),
                           0.0f, 1.0f };
    cmd.setViewport(0, viewport);
    cmd.setScissor(0, vk::Rect2D{ {0, 0}, {spec_.width, spec_.height} });
  }

  void VKFramebuffer::endRenderPass(vk::CommandBuffer cmd) const {
    cmd.endRenderPass();
  }


}