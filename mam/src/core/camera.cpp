
#include "core/camera.hpp"
#include "core/input.hpp"

namespace mam
{
  Camera::Camera(InputManager& im, const glm::vec2 window_size) :
    input_(im)
  {
    is_enabled_ = true;
    speed_ = 150.0f;
    sensitivity_ = 0.1f;
    accum_mouse_offset_ = {0.0f, 0.0f};
    view_direction_ = {0.0f, 0.0f, -5.0f};
    position_ = {5.0f, 5.0f, 5.0f};
    up_ = {0.0f, 1.0f, 0.0f};
    front_ = {1.0f, 0.0f, 0.0f};
    last_mouse_position_ = {window_size.x / 2.0f, window_size.y / 2.0f};
    aspect_ratio_ = window_size.x / window_size.y;
    first_frame_ = true;
    
    zNear_ = 0.1f;
    zFar_ = 30000.0f;

    yaw_ = -90.0f;
    pitch_ = 0.0f;

    fov_ = 45.0f;
  }

  Camera::~Camera()
  {
    is_enabled_ = false;
  }

  void Camera::initViewTarget(const glm::vec3& target, const glm::vec2 window_size)
  {

    glm::vec3 dir = target - position_;
    float len = glm::length(dir);
    if (len < 0.00001f) { return; } 

    glm::vec3 direction = glm::normalize(dir);
    front_ = direction;
    view_direction_ = direction;

    pitch_ = glm::degrees(asin(glm::clamp(direction.y, -1.0f, 1.0f)));
    yaw_ = glm::degrees(atan2(direction.z, direction.x));

    if (pitch_ > 89.0f) pitch_ = 89.0f;
    if (pitch_ < -89.0f) pitch_ = -89.0f;

    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 cameraRight = glm::normalize(glm::cross(worldUp, front_));
    up_ = glm::normalize(glm::cross(front_, cameraRight));

    last_mouse_position_ = { window_size.x / 2.0f, window_size.y / 2.0f };
    accum_mouse_offset_ = { 0.0f, 0.0f };
    first_frame_ = true;
  }

  void Camera::update(const double dt, const glm::vec2 window_size, bool canMove)
  {
    if (!is_enabled_)
      return;

    aspect_ratio_ = window_size.x / window_size.y;
    
    glm::vec2 current_mouse_position = input_.mousePosition();

    if (first_frame_)
    {
      last_mouse_position_ = current_mouse_position;
      first_frame_ = false;
    }

    glm::vec2 mouse_offset = glm::vec3{0.0f, 0.0f, 0.0f};

    glm::vec3 direction = glm::vec3{0.0f, 0.0f, 0.0f};
  
    if (input_.isMouseButtonPressed(1) && canMove)
    {

      mouse_offset = {
          (current_mouse_position.x - last_mouse_position_.x) * sensitivity_,
          (last_mouse_position_.y - current_mouse_position.y) * sensitivity_};

      if (input_.isKeyPressed(W))
      {
        direction += front_;
      }

      if (input_.isKeyPressed(S))
      {
        direction -= front_;
      }

      if (input_.isKeyPressed(A))
      {
        direction -= glm::normalize(glm::cross(front_, up_));
      }
    
      if (input_.isKeyPressed(D))
      {
        direction += glm::normalize(glm::cross(front_, up_));
      }

      if (input_.isKeyPressed(Q))
      {
        direction += glm::vec3{0.0f, -1.0f, 0.0f};
      }
      
      if (input_.isKeyPressed(E))
      {
        direction += glm::vec3{0.0f, 1.0f, 0.0f};
      }
    }

    last_mouse_position_ = current_mouse_position;

    direction *= speed_ * dt;
    position_ += direction;

    yaw_ += mouse_offset.x;
    pitch_ += mouse_offset.y;

    if (pitch_ > 89.0f)
      pitch_ = 89.0f;
    if (pitch_ < -89.0f)
      pitch_ = -89.0f;

    view_direction_.x = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    view_direction_.y = sin(glm::radians(pitch_));
    view_direction_.z = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    front_ = glm::normalize(view_direction_);

    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 cameraRight = glm::normalize(glm::cross(worldUp, front_));

    up_ = glm::normalize(glm::cross(front_, cameraRight));

    if (canMove) {
      float scroll_offset = input_.getMouseScrollOffset(); 
      fov_ -= scroll_offset; 
      if (fov_ < 1.0f) fov_ = 1.0f;      
      if (fov_ > 90.0f) fov_ = 90.0f;
    }
  }
  
}