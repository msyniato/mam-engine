
#include "render/vulkan/vk_context.hpp"
#include "render/vulkan/vk_device.hpp"
#include "render/vulkan/vk_window.hpp"

const int MAX_FRAMES_IN_FLIGHT = 2;

namespace mam {


	VKContext::VKContext() {

	}

  void VKContext::init(Window* window) {
		graphicsDevice_ = std::make_unique<VKDevice>(window);
		assert(graphicsDevice_ && "Failed to create graphics device!");

		graphicsDevice_->createInstance();
		graphicsDevice_->setupDebugCallback();
		graphicsDevice_->createSurface();
		graphicsDevice_->pickPhysicalDevice();
		graphicsDevice_->createLogicalDevice();
		graphicsDevice_->createSwapChain();
		graphicsDevice_->createImageViews();
		graphicsDevice_->createCommandPool();

		graphicsDevice_->createRenderPass();
		graphicsDevice_->createFramebuffers();

		graphicsDevice_->createCommandBuffers();
		graphicsDevice_->createSyncObjects();

  }

  void VKContext::beginFrame() {
		vk::Result waitResult = graphicsDevice_->device->waitForFences(
			1, &graphicsDevice_->inFlightFences[graphicsDevice_->currentFrame],
			VK_TRUE, std::numeric_limits<uint64_t>::max()
		);

		if (waitResult != vk::Result::eSuccess) {
			throw std::runtime_error("failed to wait for Vulkan fence");
		}

		u32 oldIndex = graphicsDevice_->imageIndex;

		try {
			vk::ResultValue result = graphicsDevice_->device->acquireNextImageKHR(graphicsDevice_->swapChain, 
				std::numeric_limits<uint64_t>::max(),
				graphicsDevice_->imageAvailableSemaphores[graphicsDevice_->currentFrame], nullptr);

			graphicsDevice_->imageIndex = result.value;
		}catch (vk::OutOfDateKHRError err) {
			graphicsDevice_->recreateSwapChain();
			return;
		}catch (vk::SystemError err) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		vk::CommandBufferBeginInfo beginInfo = {};
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;

		try {
			graphicsDevice_->getCommandBuffer().reset(vk::CommandBufferResetFlagBits::eReleaseResources);
			graphicsDevice_->getCommandBuffer().begin(beginInfo);
		}catch (vk::SystemError err) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

  }

  void VKContext::endFrame() {
		try {
			graphicsDevice_->getCommandBuffer().end();
		}
		catch (vk::SystemError err) {
			throw std::runtime_error("failed to record command buffer!");
		}

		u32 oldIndex = graphicsDevice_->imageIndex;

		vk::SubmitInfo submitInfo = {};
		vk::Semaphore waitSemaphores[] = { graphicsDevice_->imageAvailableSemaphores[graphicsDevice_->currentFrame] };
		vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &graphicsDevice_->commandBuffers[graphicsDevice_->imageIndex];

		vk::Semaphore signalSemaphores[] = { 
			graphicsDevice_->renderFinishedSemaphores[graphicsDevice_->currentFrame] };

		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vk::Result resetResult = graphicsDevice_->device->resetFences(
			1, &graphicsDevice_->inFlightFences[graphicsDevice_->currentFrame]
		);

		if (resetResult != vk::Result::eSuccess) {
			throw std::runtime_error("failed to reset Vulkan fence");
		}

		try {
			graphicsDevice_->graphicsQueue.submit(submitInfo, 
				graphicsDevice_->inFlightFences[graphicsDevice_->currentFrame]);
		}
		catch (vk::SystemError err) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		vk::PresentInfoKHR presentInfo = {};
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		vk::SwapchainKHR swapChains[] = { graphicsDevice_->swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &graphicsDevice_->imageIndex;

		vk::Result resultPresent;
		try {
			resultPresent = graphicsDevice_->presentQueue.presentKHR(presentInfo);
		}
		catch (vk::OutOfDateKHRError err) {
			resultPresent = vk::Result::eErrorOutOfDateKHR;
		}
		catch (vk::SystemError err) {
			throw std::runtime_error("failed to present swap chain image!");
		}
		VKWindow* vkWindow = static_cast<VKWindow*>(graphicsDevice_->window_);
		if (resultPresent == vk::Result::eErrorOutOfDateKHR ||
			resultPresent == vk::Result::eSuboptimalKHR ||
			(vkWindow && vkWindow->framebufferResized_)) {
			if (vkWindow)vkWindow->framebufferResized_ = false;
			graphicsDevice_->framebufferResized = false;
			graphicsDevice_->recreateSwapChain();
			return;
		}

		graphicsDevice_->currentFrame = (graphicsDevice_->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  void VKContext::shutdown() {
		graphicsDevice_->cleanup();
  }

  GraphicsDevice* VKContext::getGraphicsDevice() { return graphicsDevice_.get(); }

	void VKContext::waitIdle() {
		if (!graphicsDevice_ || !graphicsDevice_->device) return;

		graphicsDevice_->device->waitIdle();

		for (auto& cb : graphicsDevice_->commandBuffers) {
			if (!cb) continue;
			try {
				cb.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
			}
			catch (...) {}
		}
	}

}