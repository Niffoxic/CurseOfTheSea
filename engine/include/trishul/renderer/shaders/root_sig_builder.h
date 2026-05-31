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
#ifndef CURSEOFTHESEA_ROOT_SIG_BUILDER_H
#define CURSEOFTHESEA_ROOT_SIG_BUILDER_H

#include <cstdint>
#include <span>
#include <vector>

#include "trishul/renderer/shaders/shader_cache.h"

namespace trishul::render::shaders
{
    //~ building the serialized root signature for a program if any stage ships
    //~ its own embedded one we just take that otherwise reflecting the cbv
    //~ bindings across every stage and bolting on the engine static samplers
    [[nodiscard]] bool build_program_root_sig(
        std::span<const shader_bytecode> stages,
        std::vector<std::uint8_t>& out_blob);

    //~ the order the engine static samplers sit in starting at register s0
    enum class static_sampler_slot : std::uint32_t
    {
        point_wrap   = 0,
        linear_wrap  = 1,
        aniso_wrap   = 2,
        point_clamp  = 3,
        linear_clamp = 4,
        aniso_clamp  = 5,
    };
} // namespace trishul::render::shaders

#endif //CURSEOFTHESEA_ROOT_SIG_BUILDER_H