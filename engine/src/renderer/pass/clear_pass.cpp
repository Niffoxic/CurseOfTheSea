//=============================================================================
// Curse of the Sea
//=============================================================================
// Created by  Niffoxic - Harsh Dubey
// Module      WM9M6 Fundamentals of Games Research Development and Management
// Institution University of Warwick
//
// A linear story driven pirate adventure built from scratch in C++23 and
// DirectX 12 for the University of Warwick game project assessment.
//=============================================================================
#include "trishul/renderer/pass/clear_pass.h"

trishul::render::passes::clear_pass::clear_pass(
    graph::resource_handle color, graph::resource_handle depth) noexcept
    : color_(color), depth_(depth)
{

}

void trishul::render::passes::clear_pass::declare(graph::declare_context &dc)
{
    dc.write(color_, graph::resource_usage::render_target);
    dc.write(depth_, graph::resource_usage::depth_write);
}

void trishul::render::passes::clear_pass::execute(const pass_context &pc)
{
    const auto& bb = pc.resources.view(color_);
    const auto& dp = pc.resources.view(depth_);

    //~ no color view nothing to clear bailing before direct cry about null handle
    if (bb.view_handle == 0) return;

    pc.ctx.set_render_target(bb.view_handle, dp.view_handle);

    //~ fixed my fav color but the purpose is to get optimized clear value instead of changing like a newbie
    pc.ctx.clear_render_target(bb.view_handle, k_clear_color);
    if (dp.view_handle != 0)
        pc.ctx.clear_depth_stencil(dp.view_handle, k_clear_depth, k_clear_stencil);
}
