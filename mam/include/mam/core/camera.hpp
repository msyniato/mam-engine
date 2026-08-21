#pragma once

#include "common/common.hpp"

namespace mam {

  class InputManager;

  /**
   * @brief First-person 3D camera with mouse-look and keyboard movement.
   *
   * Provides position, orientation, and projection parameters consumed by
   * the render system. Movement and look can be enabled or disabled at runtime.
   */
  class Camera
  {
  public:
    /**
     * @brief Construct the camera bound to a window of the given size.
     * @param im          Input manager used for mouse and keyboard queries.
     * @param window_size Rendering viewport size (width, height) in pixels.
     */
    Camera(InputManager& im, const glm::vec2 window_size);

    ~Camera();

    /**
     * @brief Initialise the camera to look toward a specific world-space target.
     * @param target      Target position in world space.
     * @param window_size Viewport size in pixels.
     */
    void initViewTarget(const glm::vec3& target, const glm::vec2 window_size);

    /**
     * @brief Update camera movement and orientation.
     * @param dt          Time elapsed since the last update (seconds).
     * @param window_size Current viewport size in pixels.
     * @param canMove     Whether movement input should be processed this frame.
     */
    void update(const double dt, const glm::vec2 window_size, bool canMove);

    /**
     * @brief Enable or disable the camera.
     * @param b True to enable, false to disable.
     */
    void setEnabled(const bool b) { is_enabled_ = b; }

    /// Returns whether the camera is currently enabled.
    bool enabled() const { return is_enabled_; }

    /**
     * @brief Set the camera's world-space position.
     * @param v New position vector.
     */
    void setPosition(const glm::vec3 v) { position_ = v; }

    /// Returns the camera's current world-space position.
    glm::vec3 position() const { return position_; }

    /**
     * @brief Set the camera's view direction.
     * @param v New view direction vector.
     */
    void setViewDirection(const glm::vec3 v) { view_direction_ = v; }

    /// Returns the current view direction vector.
    glm::vec3 viewDirection() const { return view_direction_; }

    /// Returns the camera's normalised forward vector.
    glm::vec3 front() const { return front_; }

    /// Returns the camera's normalised up vector.
    glm::vec3 up() const { return up_; }

    /**
     * @brief Set the camera movement speed.
     * @param s Speed in world units per second.
     */
    void setSpeed(const float s) { speed_ = s; }

    /// Returns the camera movement speed.
    float speed() const { return speed_; }

    /**
     * @brief Set the mouse look sensitivity.
     * @param s Sensitivity multiplier.
     */
    void setSensitivity(const float s) { sensitivity_ = s; }

    /// Returns the mouse look sensitivity.
    float sensitivity() const { return sensitivity_; }

    /// Returns the field of view in degrees.
    float fov() const { return fov_; }

    /// Returns the aspect ratio (width / height).
    float aspectRatio() const { return aspect_ratio_; }

    /**
     * @brief Set the near clipping plane distance.
     * @param f Near plane distance.
     */
    void setZNear(const float f) { zNear_ = f; }

    /// Returns the near clipping plane distance.
    float zNear() const { return zNear_; }

    /**
     * @brief Set the far clipping plane distance.
     * @param f Far plane distance.
     */
    void setZFar(const float f) { zFar_ = f; }

    /// Returns the far clipping plane distance.
    float zFar() const { return zFar_; }

  protected:
    InputManager& input_; ///< Input manager used for movement and look queries.

    glm::vec3 view_direction_;      ///< Direction the camera is currently looking.
    glm::vec3 position_;            ///< World-space position.
    glm::vec3 front_;               ///< Normalised forward vector.
    glm::vec3 up_;                  ///< Normalised up vector.

    glm::vec2 last_mouse_position_; ///< Mouse position recorded on the previous frame.
    glm::vec2 accum_mouse_offset_;  ///< Accumulated mouse delta for smooth look.

    float aspect_ratio_; ///< Viewport aspect ratio (width / height).
    float sensitivity_;  ///< Mouse look sensitivity multiplier.
    float speed_;        ///< Movement speed in world units per second.
    float pitch_;        ///< Pitch angle in degrees (rotation around X).
    float fov_;          ///< Vertical field of view in degrees.
    float yaw_;          ///< Yaw angle in degrees (rotation around Y).

    float zNear_; ///< Near clipping plane distance.
    float zFar_;  ///< Far clipping plane distance.

    bool first_frame_; ///< True on the first update frame (suppresses jump from accumulated mouse delta).
    bool is_enabled_;  ///< Whether the camera processes input and updates.
  };

} // namespace mam
