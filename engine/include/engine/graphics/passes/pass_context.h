// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_PASS_CONTEXT_H
#define CURSEOFTHESEA_PASS_CONTEXT_H

#include "engine/graphics/render_snapshot.h"
#include "engine/graphics/hardware/command_context.h"

struct ID3D12Resource2;

namespace cots::graphics::graph
{
    struct pass_context
    {
        hardware::command_context&  command;
        const scene_snapshot&       snapshot;

        //~ TODO: Generalize this to named resources
        ID3D12Resource2* backbuffer{};
        std::size_t      rtv_handle{};

        std::uint32_t width      {};
        std::uint32_t height     {};
        std::uint32_t frame_index{};
    };
} // namespace cots::graphics::graph

#endif //CURSEOFTHESEA_PASS_CONTEXT_H
