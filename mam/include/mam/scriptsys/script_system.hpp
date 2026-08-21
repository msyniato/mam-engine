#pragma once

#include "../common/common.hpp"
#include "../ecs/entity.hpp"
#include "sol/sol.hpp"

namespace mam {

  class World;
  class Context;
  class InputManager;
  struct ScriptComponent;

  /**
   * @brief Lua scripting subsystem.
   *
   * Manages a shared sol2 Lua state, binds the engine API to Lua,
   * and drives per-entity script components each frame.
   * Implemented as a singleton accessed via the static @ref tick entry point.
   */
  class ScriptSystem {
  public:
    ScriptSystem() = default;
    ~ScriptSystem() = default;

    ScriptSystem(const ScriptSystem&) = delete;
    ScriptSystem& operator=(const ScriptSystem&) = delete;

    /**
     * @brief Initialise the scripting subsystem and bind the engine API to Lua.
     * @param world ECS world used to query and update script components.
     * @param input Input manager exposed to Lua scripts.
     */
    void init(World* world, InputManager* input);

    /**
     * @brief Shut down the scripting subsystem and release the Lua state.
     */
    void shutdown();

    /**
     * @brief ECS system entry point — tick all active script components.
     * @param ctx ECS context providing access to subsystems.
     */
    static void tick(Context& ctx);

    /// Returns the underlying Lua state.
    sol::state& lua() { return lua_; }

  private:
    /**
     * @brief Internal per-frame tick implementation.
     * @param ctx ECS context.
     */
    void doTick(Context& ctx);

    /**
     * @brief Bind the engine C++ API to the Lua state.
     */
    void bindAPI();

    /**
     * @brief Ensure the Lua script for the given entity is loaded.
     * @param e Entity whose ScriptComponent needs loading.
     */
    void ensureLoaded(Entity e);

    static ScriptSystem* s_instance; ///< Singleton instance pointer.

    World*        world_ = nullptr; ///< ECS world used to iterate script components.
    InputManager* input_ = nullptr; ///< Input manager exposed to scripts.
    sol::state    lua_;             ///< Shared Lua state for all scripts.
  };

} // namespace mam
