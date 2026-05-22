// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/passes/present_pass.h"
#include "engine/graphics/hardware/types.h"
#include "engine/utils/profiler.h"

namespace cots::graphics::passes
{
    present_pass::present_pass(const graph::resource_handle backbuffer,
                               const graph::resource_handle depth) noexcept
        : backbuffer_(backbuffer), depth_(depth)
    {}

    void present_pass::declare(graph::declare_context& dc)
    {
        //~ reads to validate that an earlier pass wrote them
        dc.read(backbuffer_);
        dc.read(depth_);
    }

    void present_pass::execute(const pass_context& pc)
    {
        COTS_PROFILE_SCOPE("present_pass::execute");

        const auto& bb = pc.resources.view(backbuffer_);
        const auto& dp = pc.resources.view(depth_);

        pc.ctx.transition(bb.resource,
                          hardware::resource_state::render_target,
                          hardware::resource_state::present);

        if (dp.resource)
        {
            pc.ctx.transition(dp.resource,
                              hardware::resource_state::depth_write,
                              hardware::resource_state::common);
        }
    }
} // namespace cots::graphics::passes
