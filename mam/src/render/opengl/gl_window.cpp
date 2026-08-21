
#include "render/opengl/gl_window.hpp"

mam::GLWindow::GLWindow(const u16 w, const u16 h)
{
  glfwInit();

	width_ = w;
	height_ = h;
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
  
  glfw_window_ = glfwCreateWindow(width_, height_, "MAMEngine", nullptr, nullptr);
  
  glfwMakeContextCurrent(glfw_window_);
  
  GLFWimage images[2];
  images[0].pixels = stbi_load("../assets/interface/icon/icon16.png", &images[0].width, &images[0].height, 0, 4);
  images[1].pixels = stbi_load("../assets/interface/icon/icon32.png", &images[1].width, &images[1].height, 0, 4);

  glfwSetWindowIcon(glfw_window_, 2, images);
  
  stbi_image_free(images[0].pixels);
  stbi_image_free(images[1].pixels);

  if (glewInit() != GLEW_OK)
  {
    abort();
  }
}

mam::GLWindow::~GLWindow()
{
  glfwDestroyWindow(glfw_window_);
}

u16 mam::GLWindow::getHeight() {
  return height_;
}

u16 mam::GLWindow::getWidth() {
  return width_;
}

void mam::GLWindow::processEvents()
{
  int w, h;
  glfwGetWindowSize(glfw_window_, &w, &h);

  width_  = static_cast<u16>(w);
  height_ = static_cast<u16>(h);
  
  glfwPollEvents();
}

void mam::GLWindow::swap()
{
  glfwSwapBuffers(glfw_window_);
}

void mam::GLWindow::render()
{
  swap();
  processEvents();
}

bool mam::GLWindow::isClosed()
{
  return glfwWindowShouldClose(glfw_window_) == GL_TRUE;
}

void mam::GLWindow::cleanup()
{
  glfwTerminate();
}