#include "ui/vk_imgui_wrapper.hpp"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include "ecs/system.hpp"
#include "render/vulkan/vk_context.hpp"
#include "render/vulkan/vk_device.hpp"
#include "render/vulkan/vk_framebuffer.hpp"
#include "render/vulkan/vk_texture.hpp"

namespace mam {

  static VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;

  void VKImGUIWrapper::initImplementation(GLFWwindow* window) {
    auto* vkContext = dynamic_cast<VKContext*>(graphicsContext_);
    assert(vkContext && "VKImGUIWrapper requires VKContext");

    auto* vkDevice = dynamic_cast<VKDevice*>(vkContext->getGraphicsDevice());
    assert(vkDevice && "VKImGUIWrapper requires VKDevice");

    ImGui_ImplGlfw_InitForVulkan(window, true);

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;

    VkDescriptorPoolSize pool_sizes[] = {
      { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
      { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
      { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
      { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(pool_sizes));
    pool_info.pPoolSizes = pool_sizes;

    vkCreateDescriptorPool(
      vkDevice->device.get(),
      &pool_info,
      nullptr,
      &g_DescriptorPool
    );

    QueueFamilyIndices indices =
      vkDevice->findQueueFamilies(vkDevice->physicalDevice);

    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.ApiVersion = VK_API_VERSION_1_0;
    init_info.Instance = static_cast<VkInstance>(vkDevice->instance.get());
    init_info.PhysicalDevice = static_cast<VkPhysicalDevice>(vkDevice->physicalDevice);
    init_info.Device = vkDevice->device.get();
    init_info.QueueFamily = indices.graphicsFamily.value();
    init_info.Queue = static_cast<VkQueue>(vkDevice->graphicsQueue);
    init_info.DescriptorPool = g_DescriptorPool;
    init_info.MinImageCount = 2;
    init_info.ImageCount = static_cast<uint32_t>(vkDevice->swapChainImageViews.size());
    if (init_info.ImageCount < 2) {
      init_info.ImageCount = 2;
    }

    vk::RenderPass imguiRenderPass = vkDevice->renderPass;
    assert(imguiRenderPass && "VKImGUIWrapper: currentRenderPass is null");

    init_info.PipelineInfoMain.RenderPass =
      static_cast<VkRenderPass>(imguiRenderPass);

    init_info.PipelineInfoMain.Subpass = 0;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    init_info.UseDynamicRendering = false;

    assert(init_info.ImageCount >= init_info.MinImageCount && "VKImGUIWrapper: invalid ImageCount");

    ImGui_ImplVulkan_Init(&init_info);
  }

  void VKImGUIWrapper::uploadFonts() {

  }

  void VKImGUIWrapper::beginFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
  }

  void VKImGUIWrapper::endFrame() {
    auto* vkContext = dynamic_cast<VKContext*>(graphicsContext_);
    assert(vkContext && "VKImGUIWrapper requires VKContext");
    auto* vkDevice = dynamic_cast<VKDevice*>(vkContext->getGraphicsDevice());
    assert(vkDevice && "VKImGUIWrapper requires VKDevice");

    ImGui::Render();

    if (vkDevice->swapChainFramebuffers.empty() ||
      vkDevice->imageIndex >= vkDevice->swapChainFramebuffers.size() ||
      !vkDevice->renderPass) {
      return;
    }

    vk::CommandBuffer cmd = vkDevice->getCommandBuffer();

    assert(!vkDevice->swapChainFramebuffers.empty() && "swapChainFramebuffers is empty!");
    assert(vkDevice->imageIndex < vkDevice->swapChainFramebuffers.size() && "imageIndex out of range!");

    vk::RenderPassBeginInfo rpInfo{};
    rpInfo.renderPass = vkDevice->renderPass;
    rpInfo.framebuffer = vkDevice->swapChainFramebuffers[vkDevice->imageIndex];
    rpInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
    rpInfo.renderArea.extent = vkDevice->swapChainExtent;

    vk::ClearValue clear{};
    clear.color = vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f });
    rpInfo.clearValueCount = 1;
    rpInfo.pClearValues = &clear;

    cmd.beginRenderPass(rpInfo, vk::SubpassContents::eInline);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), static_cast<VkCommandBuffer>(cmd));
    cmd.endRenderPass();
  }

  void VKImGUIWrapper::shutdownImplementation() {
    auto* vkContext = dynamic_cast<VKContext*>(graphicsContext_);
    if (!vkContext) return;

    auto* vkDevice = dynamic_cast<VKDevice*>(vkContext->getGraphicsDevice());
    if (!vkDevice) return;

    vkDevice->device->waitIdle();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    if (g_DescriptorPool) {
      vkDestroyDescriptorPool(vkDevice->device.get(), g_DescriptorPool, nullptr);
      g_DescriptorPool = VK_NULL_HANDLE;
    }
  }

  void VKImGUIWrapper::rebuildViewportDescriptor(VKDevice* vkDevice, FrameBuffer* fb) {
    auto* vkFb = dynamic_cast<VKFramebuffer*>(fb);
    if (!vkFb) return;

    auto& colorAttachments = vkFb->getVKColorAttachments();
    if (colorAttachments.empty()) return;

    auto& tex = colorAttachments[0];
    if (!tex) return;

    if (viewportDescriptorSet_) {
      ImGui_ImplVulkan_RemoveTexture(viewportDescriptorSet_);
      viewportDescriptorSet_ = VK_NULL_HANDLE;
    }

    viewportDescriptorSet_ = ImGui_ImplVulkan_AddTexture(
      tex->sampler(),
      tex->imageView(),
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    lastViewportW_ = vkFb->spec().width;
    lastViewportH_ = vkFb->spec().height;
  }

  void VKImGUIWrapper::renderViewport(Context& context) {
    ImGuiWindowFlags viewport_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove;
    ImGui::Begin("Scene", nullptr, viewport_flags);

    ImVec2 size = ImGui::GetContentRegionAvail();
    isCameraMovable_ = ImGui::IsWindowHovered();
    lastViewportSize_ = { size.x, size.y };

    auto* fb = context.Get<FrameBuffer>();
    if (!fb || fb->getColorAttachments().empty()) {
      ImGui::Text("Framebuffer not ready.");
      ImGui::End();
      return;
    }

    auto* vkContext = dynamic_cast<VKContext*>(graphicsContext_);
    auto* vkDevice = vkContext ? dynamic_cast<VKDevice*>(vkContext->getGraphicsDevice()) : nullptr;

    if (!vkDevice) {
      ImGui::Text("No Vulkan device.");
      ImGui::End();
      return;
    }

    uint32_t w = static_cast<uint32_t>(size.x);
    uint32_t h = static_cast<uint32_t>(size.y);

    if (!viewportDescriptorSet_ || w != lastViewportW_ || h != lastViewportH_) {
      rebuildViewportDescriptor(vkDevice, fb);
    }

    if (viewportDescriptorSet_) {
      ImGui::Image(
        (ImTextureID)viewportDescriptorSet_,
        size,
        ImVec2(0, 1), 
        ImVec2(1, 0)
      );
    }
    else {
      ImGui::Text("Viewport descriptor not ready.");
    }

    ImGui::End();
  }


}