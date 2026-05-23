// Created by Niffoxic (Harsh Dubey)
#include "editor/passes/editor_pass.h"
#include "editor/editor.h"

#include "engine/graphics/graph/declare_context.h"

namespace cots::editor
{
    editor_pass::editor_pass(graphics::graph::resource_handle backbuffer) noexcept
        : backbuffer_(backbuffer) {}

    void editor_pass::declare(graphics::graph::declare_context& dc)
    {
        dc.write(backbuffer_, graphics::graph::resource_usage::render_target);
    }

    void editor_pass::execute(const graphics::pass_context& pc)
    {
        frame_step(pc, backbuffer_);
    }
} // namespace cots::editor
