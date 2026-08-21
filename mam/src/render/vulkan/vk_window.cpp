
#include "render/vulkan/vk_window.hpp"
#include "render/api/graphics_context.hpp"
#include "render/vulkan/vk_device.hpp"

namespace mam {
	
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto* vkWindow = static_cast<VKWindow*>(glfwGetWindowUserPointer(window));
		if (!vkWindow) return;
		vkWindow->framebufferResized_ = true;
	}

	VKWindow::VKWindow(const uint16_t w, const uint16_t h)
	{
		glfwInit();

		width_ = w;
		height_ = h;
	
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
	
		glfw_window_ = glfwCreateWindow(width_, height_, "VulkanWindow", nullptr, nullptr);

		glfwSetWindowUserPointer(glfw_window_, this);
		glfwSetFramebufferSizeCallback(glfw_window_, framebufferResizeCallback);

	}

	u16 VKWindow::getHeight() {
		return height_;
	}

	u16 VKWindow::getWidth() {
		return width_;
	}

	VKWindow::~VKWindow()
	{
		if (glfw_window_) {
			glfwDestroyWindow(glfw_window_);
			glfw_window_ = nullptr;
		}
	}

	void VKWindow::processEvents()
	{
		glfwPollEvents();
	}

	void VKWindow::swap(){}

	void VKWindow::render()
	{
		swap();
		processEvents();
	}

	bool VKWindow::isClosed()
	{
		return !glfw_window_ || glfwWindowShouldClose(glfw_window_) == GL_TRUE;
	}

	void VKWindow::cleanup()
	{
		if (glfw_window_) {
			glfwDestroyWindow(glfw_window_);
			glfw_window_ = nullptr;
		}
		glfwTerminate();
	}

}