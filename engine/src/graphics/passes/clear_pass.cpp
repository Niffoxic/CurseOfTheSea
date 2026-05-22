// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/passes/clear_pass.h"
#include "engine/graphics/hardware/types.h"
#include "engine/utils/profiler.h"

#include <cmath>

namespace cots::graphics::passes
{
    void clear_pass::execute(const pass_context& pc)
    {
        COTS_PROFILE_SCOPE("clear_pass::execute");

        pc.ctx.transition(pc.backbuffer,
                          hardware::resource_state::present,
                          hardware::resource_state::render_target);

        pc.ctx.set_render_target(pc.rtv_handle);

        //~ snapshot-driven color
        const float t = static_cast<float>(pc.snap.frame_id) * 0.01f;
        const float color[4] =
        {
            0.5f + 0.5f * std::sin(t * 0.7f),
            0.5f + 0.5f * std::sin(t * 0.9f + 2.0f),
            0.5f + 0.5f * std::sin(t * 1.3f + 4.0f),
            1.0f
        };
        pc.ctx.clear_render_target(pc.rtv_handle, color);
    }
} // namespace cots::graphics::graph
