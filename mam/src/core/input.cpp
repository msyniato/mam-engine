

#include "core/input.hpp"
#include "core/framework.hpp"
#include "render/api/window.hpp"

namespace mam
{

  float InputManager::scrollOffsetY_ = 0.0f;

  InputManager::InputManager(GLFWwindow* window) :
    window_(window)
  {}
  
  void InputManager::init() {
    glfwSetWindowUserPointer(window_, this);
    glfwSetScrollCallback(window_, scrollCallback);
  }
  
  bool InputManager::isKeyPressed(KeyCode key)
  {
    auto state = glfwGetKey(window_, key);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
  }

  bool InputManager::isKeyReleased(KeyCode key)
  {
    auto state = glfwGetKey(window_, key);
    return state == GLFW_RELEASE;
  }

  bool InputManager::isKeyRepeated(KeyCode key)
  {
    auto state = glfwGetKey(window_, key);
    return state == GLFW_REPEAT;
  }

  bool InputManager::isMouseButtonPressed(MouseCode key)
  {
    auto state = glfwGetMouseButton(window_, key);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
  }

  bool InputManager::isMouseButtonReleased(MouseCode key)
  {
    auto state = glfwGetMouseButton(window_, key);
    return state == GLFW_RELEASE;
  }

  float InputManager::getMouseScrollOffset()
  {
    float offset = (float)scrollOffsetY_;
    scrollOffsetY_ = 0.0; 
    return offset;
  }

  void InputManager::showMouse(bool show)
  {
    if (show)
    {
      glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    else
    {
      glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }
  }

  glm::vec2 InputManager::mousePosition()
  {
    double x, y;
    glfwGetCursorPos(window_, &x, &y); 
    return {(float)x, (float)y};
  }

  float InputManager::mouseX()
  {
    return mousePosition().x;
  }

  float InputManager::mouseY()
  {
    return mousePosition().y;
  }

  void InputManager::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
  {
    scrollOffsetY_ += (float)yoffset;
  }
}
