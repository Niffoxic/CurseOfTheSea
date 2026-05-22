// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_ROOT_SIG_BUILDER_H
#define CURSEOFTHESEA_ROOT_SIG_BUILDER_H

#include <cstdint>
#include <span>
#include <vector>

#include "engine/graphics/shaders/shader_cache.h"

namespace cots::graphics::shaders
{
    //~ build the serialized root signature
    //~ if any stage has an embedded root sig use that
    //~ otherwise reflecting cbv bindings and add static samplers
    [[nodiscard]] bool build_program_root_sig(
        std::span<const shader_bytecode> stages,
        std::vector<std::uint8_t>& out_blob);

    //~ engine static samplers register order
    enum class static_sampler_slot : std::uint32_t
    {
        point_wrap   = 0,
        linear_wrap  = 1,
        aniso_wrap   = 2,
        point_clamp  = 3,
        linear_clamp = 4,
        aniso_clamp  = 5,
    };
} // namespace cots::graphics::shaders

#endif //CURSEOFTHESEA_ROOT_SIG_BUILDER_H
