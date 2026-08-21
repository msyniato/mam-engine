#include "scriptsys/script_system.hpp"
#include "ecs/world.hpp"
#include "ecs/engine_components.hpp"
#include "core/input.hpp"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

namespace mam {

  ScriptSystem* ScriptSystem::s_instance = nullptr;

  void ScriptSystem::init(World* world, InputManager* input) {
    world_ = world;
    input_ = input;
    s_instance = this;

    if (world_) {
      world_->registerComponent<ScriptComponent>();
    }

    lua_.open_libraries(
      sol::lib::base,
      sol::lib::math,
      sol::lib::table,
      sol::lib::string
    );

    bindAPI();
  }

  void ScriptSystem::shutdown() {
    if (!world_) {
      if (s_instance == this) s_instance = nullptr;
      return;
    }

    auto scType = world_->getComponentType<ScriptComponent>();

    for (auto& arch : world_->getArchetypes()) {
      if (!arch->signature().test(scType)) continue;

      const std::vector<Entity> entities = arch->entities();
      for (Entity e : entities) {
        auto* sc = world_->getComponent<ScriptComponent>(e);
        if (!sc || !sc->ctx) continue;

        if (sc->ctx->destroy.valid()) {
          sol::protected_function pf = sc->ctx->destroy;
          sol::protected_function_result r = pf(e);
          if (!r.valid()) {
            sol::error err = r;
            std::cerr << "[Lua] destroy error (" << sc->file << "): "
              << err.what() << "\n";
          }
        }
        sc->ctx.reset();
      }
    }

    if (s_instance == this) s_instance = nullptr;
  }

  void ScriptSystem::tick(Context& ctx) {
    if (s_instance) s_instance->doTick(ctx);
  }

  void ScriptSystem::doTick(Context& ctx) {
    World* world = ctx.Get<World>();
    if (!world) world = world_; 
    if (!world) return;

    auto scType = world->getComponentType<ScriptComponent>();

    for (auto& arch : world->getArchetypes()) {
      if (!arch->signature().test(scType)) continue;

      const std::vector<Entity> entities = arch->entities();
      for (Entity e : entities) {
        ensureLoaded(e);

        auto* sc = world->getComponent<ScriptComponent>(e);
        if (!sc || !sc->ctx) continue;

        if (sc->ctx->update.valid()) {
          sol::protected_function pf = sc->ctx->update;
          sol::protected_function_result r = pf(e, world->lastDeltaTime);
          if (!r.valid()) {
            sol::error err = r;
            std::cerr << "[Lua] update error (" << sc->file << "): "
              << err.what() << "\n";
          }
        }
      }
    }
  }

  void ScriptSystem::ensureLoaded(Entity e) {
    if (!world_) return;

    auto* sc = world_->getComponent<ScriptComponent>(e);
    if (!sc) return;

    if (!sc->ctx) {
      sc->ctx = std::make_shared<ScriptContext>(lua_);

      sol::protected_function_result r =
        lua_.safe_script_file(sc->file, sc->ctx->env);

      if (!r.valid()) {
        sol::error err = r;
        std::cerr << "[Lua] error cargando " << sc->file << ": "
          << err.what() << "\n";
        sc->ctx.reset();
        return;
      }

      sc->ctx->init = sc->ctx->env["init"];
      sc->ctx->update = sc->ctx->env["update"];
      sc->ctx->destroy = sc->ctx->env["destroy"];
    }

    if (sc->ctx && !sc->ctx->initialized) {
      if (sc->ctx->init.valid()) {
        sol::protected_function pf = sc->ctx->init;
        sol::protected_function_result r = pf(e);
        if (!r.valid()) {
          sol::error err = r;
          std::cerr << "[Lua] init error (" << sc->file << "): "
            << err.what() << "\n";
        }
      }
      sc->ctx->initialized = true;
    }
  }

  void ScriptSystem::bindAPI() {

    lua_.new_usertype<glm::vec3>("vec3",
      sol::constructors<glm::vec3(), glm::vec3(float, float, float)>(),
      "x", &glm::vec3::x,
      "y", &glm::vec3::y,
      "z", &glm::vec3::z
    );

    lua_.new_usertype<TransformComponent>("TransformComponent",
      "position", &TransformComponent::position,
      "rotation", &TransformComponent::rotation,
      "scale", &TransformComponent::scale
    );

    lua_.new_usertype<Entity>("Entity",
      sol::constructors<Entity(), Entity(ID)>(),
      "id", sol::property([](Entity& e) { return e.id(); }),
      "isValid", &Entity::isValid
    );

    auto keyTable = lua_.create_table();
    keyTable["Up"] = static_cast<int>(Up);
    keyTable["Down"] = static_cast<int>(Down);
    keyTable["Left"] = static_cast<int>(Left);
    keyTable["Right"] = static_cast<int>(Right);
    keyTable["Space"] = static_cast<int>(Space);
    keyTable["W"] = static_cast<int>(W);
    keyTable["A"] = static_cast<int>(A);
    keyTable["S"] = static_cast<int>(S);
    keyTable["D"] = static_cast<int>(D);
    lua_["Key"] = keyTable;

    auto inputTable = lua_.create_table();
    inputTable["isKeyPressed"] = [this](int key) -> bool {
      if (!input_) return false;
      return input_->isKeyPressed(static_cast<KeyCode>(key));
      };
    lua_["Input"] = inputTable;

    auto ecs = lua_.create_table();

    ecs["createEntity"] = [this](sol::optional<std::string> name) -> Entity {
      if (!world_) return kInvalidEntity;
      return world_->createEntity(name.value_or("LuaEntity"));
      };

    ecs["destroyEntity"] = [this](Entity e) {
      if (world_) world_->destroyEntity(e);
      };

    ecs["addTransform"] = [this](Entity e,
      const glm::vec3& pos,
      const glm::vec3& rot,
      const glm::vec3& scl)
      {
        if (!world_) return;
        auto* t = world_->getComponent<TransformComponent>(e);
        if (t) {
          t->position = pos;
          t->rotation = rot;
          t->scale = scl;
        }
        else {
          world_->addComponent<TransformComponent>(e,
            TransformComponent{ pos, rot, scl });
        }
      };

    ecs["removeTransform"] = [this](Entity e) {
      if (world_) world_->removeComponent<TransformComponent>(e);
      };

    ecs["getTransform"] = [this](Entity e) -> TransformComponent* {
      if (!world_) return nullptr;
      return world_->getComponent<TransformComponent>(e);
      };

    ecs["addRender"] = [this](Entity e) {
      if (world_) world_->addComponent<RenderComponent>(e, RenderComponent{});
      };

    ecs["removeRender"] = [this](Entity e) {
      if (world_) world_->removeComponent<RenderComponent>(e);
      };

    lua_["ECS"] = ecs;
  }

} // namespace mam