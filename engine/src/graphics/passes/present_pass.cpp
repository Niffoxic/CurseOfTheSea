// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/passes/present_pass.h"
#include "engine/utils/profiler.h"

namespace cots::graphics::passes
{
    present_pass::present_pass(const graph::resource_handle backbuffer) noexcept
        : backbuffer_(backbuffer)
    {}

    void present_pass::declare(graph::declare_context& dc)
    {
        dc.write(backbuffer_, graph::resource_usage::present);
    }

    void present_pass::execute(const pass_context& pc)
    {
        COTS_PROFILE_SCOPE("present_pass::execute");
    }
} // namespace cots::graphics::passes
