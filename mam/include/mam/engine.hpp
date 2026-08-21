/**
 * @file engine.hpp
 * @brief Convenience header that includes all public engine subsystems.
 *
 * Include this single header to access the full engine API from application code.
 */
#pragma once

#include "core/framework.hpp"
#include "core/input.hpp"
#include "core/camera.hpp"
#include "core/utils.hpp"

#include "common/common.hpp"

#include "render/api/buffer.hpp"
#include "render/api/graphics_context.hpp"
#include "render/api/pipeline.hpp"
#include "render/api/renderer.hpp"
#include "render/api/ssao_pass.hpp"
#include "render/api/graphics_device.hpp"
#include "render/api/shader.hpp"
#include "render/api/mesh.hpp"
#include "render/api/river.hpp"
#include "render/api/terrain.hpp"
#include "render/api/vegetation.hpp"
#include "render/api/vertex_array.hpp"
#include "render/api/window.hpp"
#include "render/api/probe_baker.hpp"
#include "render/api/probe_interpolator.hpp"

#include "ui/terrain_gen_panel.hpp"
#include "ui/imgui_wrapper.hpp"

#include "jobsys/job_system.hpp"
#include "jobsys/dispatcher.hpp"

#include "matsys/material.hpp"

#include "audiosys/audio_manager.hpp"

#include "ecs/component_array.hpp"
#include "ecs/engine_systems.hpp"
#include "ecs/engine_components.hpp"
#include "ecs/light_probe.hpp"
#include "ecs/probe_systems.hpp"
#include "ecs/world.hpp"