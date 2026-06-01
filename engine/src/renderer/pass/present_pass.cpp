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
#include "trishul/renderer/pass/present_pass.h"

trishul::render::passes::present_pass::present_pass(
    const graph::resource_handle backbuffer) noexcept
    : backbuffer_(backbuffer)
{}

void trishul::render::passes::present_pass::declare(graph::declare_context &dc)
{
    //~ just declaring the present intent
    dc.write(backbuffer_, graph::resource_usage::present);
}

void trishul::render::passes::present_pass::execute(const pass_context &pc)
{   //~ nothing to do here
    (void)pc;
}
