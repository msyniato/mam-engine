#pragma once

#include "common/common.hpp"

namespace mam {

  class Context;

  /**
   * @brief Traverses the ECS world and submits render commands for all renderable entities.
   * @param context Application context providing access to the world and render queue.
   */
  void updateRenderSystem(Context& context);

  /**
   * @brief Traverses the ECS world and collects all active light components into the renderer.
   * @param context Application context providing access to the world and renderer.
   */
  void collectLightsSystem(Context& context);

} // namespace mam
