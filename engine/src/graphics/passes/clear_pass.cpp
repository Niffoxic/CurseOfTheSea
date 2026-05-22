// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/passes/clear_pass.h"
#include "engine/graphics/hardware/types.h"
#include "engine/utils/profiler.h"

#include <cmath>

namespace cots::graphics::passes
{
    clear_pass::clear_pass(const graph::resource_handle backbuffer,
                           const graph::resource_handle depth) noexcept
        : backbuffer_(backbuffer), depth_(depth)
    {}

    void clear_pass::declare(graph::declare_context& dc)
    {
        dc.write(backbuffer_);
        dc.write(depth_);
    }

    void clear_pass::execute(const pass_context& pc)
    {
        COTS_PROFILE_SCOPE("clear_pass::execute");

        const auto& bb = pc.resources.view(backbuffer_);
        const auto& dp = pc.resources.view(depth_);

        pc.ctx.transition(bb.resource,
                          hardware::resource_state::present,
                          hardware::resource_state::render_target);

        if (dp.resource)
        {
            pc.ctx.transition(dp.resource,
                              hardware::resource_state::common,
                              hardware::resource_state::depth_write);
        }

        pc.ctx.set_render_target(bb.view_handle, dp.view_handle);

        //~ snapshot driven color
        const float t = static_cast<float>(pc.snap.frame_id) * 0.01f;
        const float color[4] =
        {
            0.5f + 0.5f * std::sin(t * 0.7f),
            0.5f + 0.5f * std::sin(t * 0.9f + 2.0f),
            0.5f + 0.5f * std::sin(t * 1.3f + 4.0f),
            1.0f
        };
        //~ clear handles for the frame
        pc.ctx.clear_render_target(bb.view_handle, color);
        pc.ctx.clear_depth_stencil(dp.view_handle, 0.0f, 0);
    }
} // namespace cots::graphics::passes
