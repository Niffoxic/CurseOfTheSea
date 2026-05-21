// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/passes/present_pass.h"
#include "engine/graphics/hardware/types.h"

namespace cots::graphics::passes
{
    void present_pass::execute(const pass_context& pc)
    {
        pc.ctx.transition(pc.backbuffer,
                          hardware::resource_state::render_target,
                          hardware::resource_state::present);
    }
} // namespace cots::graphics::graph

