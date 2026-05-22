// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/passes/present_pass.h"
#include "engine/graphics/hardware/types.h"
#include "engine/utils/profiler.h"

namespace cots::graphics::passes
{
    void present_pass::execute(const pass_context& pc)
    {
        COTS_PROFILE_SCOPE("present_pass::execute");

        pc.ctx.transition(pc.backbuffer,
                          hardware::resource_state::render_target,
                          hardware::resource_state::present);
    }
} // namespace cots::graphics::passes
