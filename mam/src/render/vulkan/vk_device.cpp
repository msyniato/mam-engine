
#include "render/vulkan/vk_device.hpp"
#include "render/api/graphics_context.hpp"
#include "render/vulkan/vk_window.hpp"
#include "render/api/mesh.hpp"

#include "render/api/buffer.hpp"
#include "render/vulkan/vk_buffer.hpp"

#include "render/api/vertex_array.hpp"
#include "render/vulkan/vk_vertex_array.hpp"

#include "render/api/shader.hpp"
#include "render/vulkan/vk_shader.hpp"

#include "render/api/pipeline.hpp"
#include "render/vulkan/vk_pipeline.hpp"

#include "render/api/frame_buffer.hpp"
#include "render/vulkan/vk_framebuffer.hpp"

#include "render/api/texture.hpp"
#include "render/vulkan/vk_texture.hpp"

#pragma region VARIABLES
const std::vector<const char*> validationLayers = {
		"VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const int MAX_FRAMES_IN_FLIGHT = 2;

#ifdef NDEBUG
static const bool enableValidationLayers = false;
#else
static const bool enableValidationLayers = false;
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

#pragma endregion

namespace mam {

	VKDevice::VKDevice(Window* window) {
		window_ = window;
	}

	void VKDevice::setCurrentPipeline(std::shared_ptr<Pipeline> pipe) {
		pipeline = std::static_pointer_cast<VKPipeline>(pipe);
	}

#pragma region GETTERS
	vk::CommandBuffer VKDevice::getCommandBuffer() {
		return commandBuffers[imageIndex];
	}

#pragma endregion

	void VKDevice::draw(Mesh& mesh) {
		mesh.vertexArray()->bind();
		getCommandBuffer().drawIndexed(static_cast<uint32_t>(mesh.indices()), 1, 0, 0, 0);
	}

	void VKDevice::draw(uint32_t indices) {
		getCommandBuffer().drawIndexed(indices, 1, 0, 0, 0);
	}

	void VKDevice::drawIndexed(uint32_t indices)
	{
		getCommandBuffer().drawIndexed(indices, 1, 0, 0, 0);
	}

	void VKDevice::enableDepthTest() {}
	void VKDevice::disableDepthTest() {}
	void VKDevice::enableFaceCulling() {}
	void VKDevice::disableFaceCulling() {}
	void VKDevice::drawInstanced(Mesh&, u32) {}

	std::unique_ptr<Shader> VKDevice::createShader(){
		return std::make_unique<VKShader>(this);
	}

	std::unique_ptr<VertexBuffer> VKDevice::createVertexBuffer(unsigned int count){
		return std::make_unique<VKVertexBuffer>(this);
	}

	std::unique_ptr<VertexBuffer> VKDevice::createVertexBuffer(void* vertex, unsigned int count){
		return std::make_unique<VKVertexBuffer>(this);
	}

	std::unique_ptr<IndexBuffer> VKDevice::createIndexBuffer(unsigned int size){
		return std::make_unique<VKIndexBuffer>(this);
	}

	std::unique_ptr<IndexBuffer> VKDevice::createIndexBuffer(void* index, unsigned int size){
		return std::make_unique<VKIndexBuffer>(this);
	}

	std::unique_ptr<Pipeline> VKDevice::createPipeline(){
		return std::make_unique<VKPipeline>(this);
	}

	std::unique_ptr<FrameBuffer> VKDevice::createFrameBuffer(const FrameBufferSpec& spec) {
		auto framebuffer = std::make_unique<VKFramebuffer>(this, spec);
		setCurrentRenderPass(framebuffer->renderPass());
		setCurrentColorAttachmentCount(static_cast<u32>(spec.colorFormats.size()));

		return framebuffer;
	}

	std::unique_ptr<VertexArray> VKDevice::createVertexArray() {
		return std::make_unique<VKVertexArray>();
	}

	std::unique_ptr<GBuffer> VKDevice::createGBuffer(glm::vec2 screen_size) {
		throw std::runtime_error("VKDevice::createGBuffer is not implemented yet");
	}

	std::unique_ptr<ShaderStorageBuffer> VKDevice::createShaderStorageBuffer(u32 pipeline_id) {
		return std::make_unique<VKStorageBuffer>(this, pipeline_id);;
	}
	
	std::unique_ptr<Texture> VKDevice::createTexture(const std::string& path) {
		return std::make_unique<VKTexture>(this, path, false);
	}

	std::unique_ptr<Texture> VKDevice::createCubemap(const std::array<std::string, 6>&) {
		throw std::runtime_error("VKDevice::createCubemap is not implemented yet");
	}

	void VKDevice::setClearColor(float r, float g, float b, float a) {}
	void VKDevice::clearBuffers() {}

	void VKDevice::registerPipeline(std::shared_ptr<VKPipeline> pipe) {
		for (auto& wp : registeredPipelines_)
			if (wp.lock() == pipe) return;
		registeredPipelines_.push_back(pipe);
	}

	void VKDevice::unregisterPipeline(VKPipeline* pipe) {
		registeredPipelines_.erase(
			std::remove_if(registeredPipelines_.begin(), registeredPipelines_.end(),
				[pipe](const std::weak_ptr<VKPipeline>& wp) {
					auto sp = wp.lock();
					return !sp || sp.get() == pipe;
				}),
			registeredPipelines_.end());
	}

#pragma region VULKAN_INIT
	void VKDevice::createInstance() {
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			//throw std::runtime_error("validation layers requested, but not available!");
		}

		auto appInfo = vk::ApplicationInfo(
			"Test",
			VK_MAKE_VERSION(1, 0, 0),
			"mam Engine",
			VK_MAKE_VERSION(1, 0, 0),
			VK_API_VERSION_1_0
		);

		auto extensions = getRequiredExtensions();
		auto createInfo = vk::InstanceCreateInfo(
			vk::InstanceCreateFlags(),
			&appInfo,
			0, nullptr, 
			static_cast<uint32_t>(extensions.size()), extensions.data()
		);

		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}

		try {
			instance = vk::createInstanceUnique(createInfo, nullptr);
		}
		catch (vk::SystemError err) {
			throw std::runtime_error("failed to create instance!");
		}

	}

	void VKDevice::setupDebugCallback() {
		if (!enableValidationLayers) return;

		auto createInfo = vk::DebugUtilsMessengerCreateInfoEXT(
			vk::DebugUtilsMessengerCreateFlagsEXT(),
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
			debugCallback,
			nullptr
		);

		if (CreateDebugUtilsMessengerEXT(*instance, reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(&createInfo), nullptr, &callback) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug callback!");
		}
	}

	void VKDevice::pickPhysicalDevice() {
		auto devices = instance->enumeratePhysicalDevices();
		if (devices.size() == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		for (const auto& device : devices) {
			if (isDeviceSuitable(device)) {
				physicalDevice = device;
				break;
			}
		}

		if (!physicalDevice) {
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	void VKDevice::createLogicalDevice() {
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

		float queuePriority = 0.0f;
		const std::vector<const char*> deviceExtensions = {
				VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		vk::PhysicalDeviceFeatures deviceFeatures = physicalDevice.getFeatures();
		vk::DeviceQueueCreateInfo deviceQueueCreateInfo({}, indices.graphicsFamily.value(), 1, &queuePriority);
		vk::DeviceCreateInfo      deviceCreateInfo({}, deviceQueueCreateInfo, {}, deviceExtensions, &deviceFeatures);
		device = physicalDevice.createDeviceUnique(deviceCreateInfo);
		graphicsQueue = device->getQueue(indices.graphicsFamily.value(), 0);
		presentQueue = device->getQueue(indices.presentFamily.value(), 0);
	}

	void VKDevice::createSurface() {
		VkSurfaceKHR rawSurface;

		GLFWwindow* w = window_->getNativeWindow();
		if (glfwCreateWindowSurface(*instance, w, nullptr, &rawSurface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}

		surface = rawSurface;
	}

	void VKDevice::createSwapChain() {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

		vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		vk::SwapchainCreateInfoKHR createInfo(
			vk::SwapchainCreateFlagsKHR(),
			surface,
			imageCount,
			surfaceFormat.format,
			surfaceFormat.colorSpace,
			extent,
			1,
			vk::ImageUsageFlagBits::eColorAttachment
		);

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = vk::SharingMode::eExclusive;
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = vk::SwapchainKHR(nullptr);

		try {
			swapChain = device->createSwapchainKHR(createInfo);
		}
		catch (vk::SystemError err) {
			throw std::runtime_error("failed to create swap chain!");
		}

		swapChainImages = device->getSwapchainImagesKHR(swapChain);

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	void VKDevice::createImageViews() {
		swapChainImageViews.resize(swapChainImages.size());

		for (size_t i = 0; i < swapChainImages.size(); i++) {
			vk::ImageViewCreateInfo createInfo = {};
			createInfo.image = swapChainImages[i];
			createInfo.viewType = vk::ImageViewType::e2D;
			createInfo.format = swapChainImageFormat;
			createInfo.components.r = vk::ComponentSwizzle::eIdentity;
			createInfo.components.g = vk::ComponentSwizzle::eIdentity;
			createInfo.components.b = vk::ComponentSwizzle::eIdentity;
			createInfo.components.a = vk::ComponentSwizzle::eIdentity;
			createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			try {
				swapChainImageViews[i] = device->createImageView(createInfo);
			}
			catch (vk::SystemError err) {
				throw std::runtime_error("failed to create image views!");
			}
		}
	}

	void VKDevice::createRenderPass() {
		vk::AttachmentDescription colorAttachment = {};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = vk::SampleCountFlagBits::e1;
		colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
		colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
		colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
		colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

		vk::AttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

		vk::SubpassDescription subpass = {};
		subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		vk::SubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;

		vk::RenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		try {
			renderPass = device->createRenderPass(renderPassInfo);
		}
		catch (vk::SystemError err) {
			throw std::runtime_error("failed to create render pass!");
		}
	}

	void VKDevice::createFramebuffers() {
		assert(renderPass && "createFramebuffers: renderPass is null");

		swapChainFramebuffers.resize(swapChainImageViews.size());

		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			vk::ImageView attachments[] = { swapChainImageViews[i] };

			vk::FramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			swapChainFramebuffers[i] = device->createFramebuffer(framebufferInfo);
		}
	}

	void VKDevice::createCommandPool() {
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

		vk::CommandPoolCreateInfo poolInfo = {};
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
		poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

		try {
			commandPool = device->createCommandPool(poolInfo);
		}
		catch (vk::SystemError err) {
			throw std::runtime_error("failed to create command pool!");
		}
	}

	void VKDevice::createCommandBuffers() {
		commandBuffers.resize(swapChainImageViews.size());

		vk::CommandBufferAllocateInfo allocInfo = {};
		allocInfo.commandPool = commandPool;
		allocInfo.level = vk::CommandBufferLevel::ePrimary;
		allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

		try {
			commandBuffers = device->allocateCommandBuffers(allocInfo);
		}
		catch (vk::SystemError err) {
			throw std::runtime_error("failed to allocate command buffers!");
		}

	}

	void VKDevice::createSyncObjects() {
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		try {
			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
				imageAvailableSemaphores[i] = device->createSemaphore({});
				renderFinishedSemaphores[i] = device->createSemaphore({});
				inFlightFences[i] = device->createFence({ vk::FenceCreateFlagBits::eSignaled });
			}
		}
		catch (vk::SystemError err) {
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}

	void VKDevice::cleanupSwapChain() {
		for (auto framebuffer : swapChainFramebuffers)
			device->destroyFramebuffer(framebuffer);
		swapChainFramebuffers.clear();

		for (auto& cb : commandBuffers)
			cb.reset(vk::CommandBufferResetFlagBits::eReleaseResources);

		device->freeCommandBuffers(commandPool, commandBuffers);
		commandBuffers.clear();

		for (auto imageView : swapChainImageViews)
			device->destroyImageView(imageView);

		device->destroySwapchainKHR(swapChain);
	}

	void VKDevice::cleanup()
	{
		if (!device) return;
		device->waitIdle();

		for (auto& wp : registeredPipelines_) {
			if (auto sp = wp.lock()) {
				sp->destroy();
			}
		}
		registeredPipelines_.clear();

		if (pipeline) {
			pipeline->destroy();
		}

		cleanupSwapChain();

		if (renderPass) {
			device->destroyRenderPass(renderPass);
			renderPass = nullptr;
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			device->destroySemaphore(renderFinishedSemaphores[i]);
			device->destroySemaphore(imageAvailableSemaphores[i]);
			device->destroyFence(inFlightFences[i]);
		}

		device->destroyCommandPool(commandPool);
		instance->destroySurfaceKHR(surface);

		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(*instance, callback, nullptr);
		}
	}

	void VKDevice::recreateSwapChain() {
		int width = 0, height = 0;
		GLFWwindow* w = window_->getNativeWindow();
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(w, &width, &height);
			glfwWaitEvents();
		}

		device->waitIdle();
		cleanupSwapChain();

		createSwapChain();
		createImageViews();
		createFramebuffers();
		createCommandBuffers();

		/*
		if (pipeline) {
			pipeline->setViewport(
				static_cast<float>(swapChainExtent.width),
				static_cast<float>(swapChainExtent.height)
			);
			pipeline->recreatePipeline();
		}

		for (auto& wp : registeredPipelines_) {
			if (auto sp = wp.lock()) {
				sp->setViewport(
					static_cast<float>(swapChainExtent.width),
					static_cast<float>(swapChainExtent.height));
				sp->recreatePipeline();
			}
		}
		*/

	}


	void VKDevice::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory) {
		vk::BufferCreateInfo bufferInfo = {};
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = vk::SharingMode::eExclusive;

		try {
			buffer = device->createBuffer(bufferInfo);
		}
		catch (vk::SystemError err) {
			throw std::runtime_error("failed to create buffer!");
		}

		vk::MemoryRequirements memRequirements = device->getBufferMemoryRequirements(buffer);

		vk::MemoryAllocateInfo allocInfo = {};
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		try {
			bufferMemory = device->allocateMemory(allocInfo);
		}
		catch (vk::SystemError err) {
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		device->bindBufferMemory(buffer, bufferMemory, 0);
	}

	void VKDevice::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		vk::CommandBufferAllocateInfo allocInfo = {};
		allocInfo.level = vk::CommandBufferLevel::ePrimary;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		vk::CommandBuffer commandBuffer = device->allocateCommandBuffers(allocInfo)[0];

		vk::CommandBufferBeginInfo beginInfo = {};
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

		commandBuffer.begin(beginInfo);

		vk::BufferCopy copyRegion = {};
		copyRegion.srcOffset = 0; 
		copyRegion.dstOffset = 0; 
		copyRegion.size = size;
		commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);

		commandBuffer.end();

		vk::SubmitInfo submitInfo = {};
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		graphicsQueue.submit(submitInfo, nullptr);
		graphicsQueue.waitIdle();

		device->freeCommandBuffers(commandPool, commandBuffer);
	}

#pragma endregion


#pragma region UTILS_FOR_FUNCTIONS

	bool VKDevice::checkValidationLayerSupport() {
		auto availableLayers = vk::enumerateInstanceLayerProperties();
		for (const char* layerName : validationLayers) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}
			if (!layerFound) {
				return false;
			}
		}
		return true;
	}

	std::vector<const char*> VKDevice::getRequiredExtensions() {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		if (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		return extensions;
	}

	VkResult VKDevice::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback) {
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pCallback);
		}else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	bool VKDevice::isDeviceSuitable(const vk::PhysicalDevice& device) {
		QueueFamilyIndices indices = findQueueFamilies(device);

		bool extensionsSupported = checkDeviceExtensionSupport(device);
		bool swapChainAdequate = false;
		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		return indices.isComplete() && extensionsSupported && swapChainAdequate;
	}

	QueueFamilyIndices VKDevice::findQueueFamilies(vk::PhysicalDevice device) {
		QueueFamilyIndices indices;
		auto queueFamilies = device.getQueueFamilyProperties();
		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
				indices.graphicsFamily = i;
			}
			if (queueFamily.queueCount > 0 && device.getSurfaceSupportKHR(i, surface)) {
				indices.presentFamily = i;
			}
			if (indices.isComplete()) {
				break;
			}
			i++;
		}
		return indices;
	}

	bool VKDevice::checkDeviceExtensionSupport(const vk::PhysicalDevice& device) {
		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension : device.enumerateDeviceExtensionProperties()) {
			requiredExtensions.erase(extension.extensionName);
		}
		return requiredExtensions.empty();
	}

	SwapChainSupportDetails VKDevice::querySwapChainSupport(const vk::PhysicalDevice& device) {
		SwapChainSupportDetails details;
		details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
		details.formats = device.getSurfaceFormatsKHR(surface);
		details.presentModes = device.getSurfacePresentModesKHR(surface);

		return details;
	}

	vk::SurfaceFormatKHR VKDevice::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
		if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined) {
			return { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
		}

		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == vk::Format::eB8G8R8A8Unorm && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	vk::PresentModeKHR VKDevice::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> availablePresentModes) {
		vk::PresentModeKHR bestMode = vk::PresentModeKHR::eFifo;

		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
				return availablePresentMode;
			}
			else if (availablePresentMode == vk::PresentModeKHR::eImmediate) {
				bestMode = availablePresentMode;
			}
		}

		return bestMode;
	}

	vk::Extent2D VKDevice::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			int width, height;
			GLFWwindow* window = window_->getNativeWindow();
			glfwGetFramebufferSize(window, &width, &height);

			vk::Extent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	vk::ShaderModule VKDevice::createShaderModule(const std::vector<char>& code) {
		try {
			return device->createShaderModule({
					vk::ShaderModuleCreateFlags(),
					code.size(),
					reinterpret_cast<const uint32_t*>(code.data())
				});
		}
		catch (vk::SystemError err) {
			throw std::runtime_error("failed to create shader module!");
		}
	}

	uint32_t VKDevice::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
		vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	void VKDevice::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance, callback, pAllocator);
		}
	}

	vk::CommandBuffer VKDevice::beginSingleTimeCommands() {
		vk::CommandBufferAllocateInfo allocInfo{};
		allocInfo.level = vk::CommandBufferLevel::ePrimary;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		vk::CommandBuffer cmd = device->allocateCommandBuffers(allocInfo)[0];

		vk::CommandBufferBeginInfo beginInfo{};
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

		cmd.begin(beginInfo);
		return cmd;
	}

	void VKDevice::endSingleTimeCommands(vk::CommandBuffer commandBuffer) {
		commandBuffer.end();

		vk::SubmitInfo submitInfo{};
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		graphicsQueue.submit(submitInfo, nullptr);
		graphicsQueue.waitIdle();

		device->freeCommandBuffers(commandPool, commandBuffer);
	}

#pragma endregion

}