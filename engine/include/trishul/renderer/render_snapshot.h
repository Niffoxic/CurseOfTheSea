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
#ifndef CURSEOFTHESEA_RENDER_SNAPSHOT_H
#define CURSEOFTHESEA_RENDER_SNAPSHOT_H

#include <cstdint>
#include <initializer_list>
#include <vector>
#include <DirectXMath.h>

namespace trishul::render
{
    struct camera_data
    {
        DirectX::XMFLOAT4X4 view_matrix      {};
        DirectX::XMFLOAT4X4 projection_matrix{};

        DirectX::XMFLOAT3 position{0.f, 0.f, 0.f};
        float near_z{  0.1f  };
        float far_z { 1000.f };
    };

    //~ particle system reads it to size the per emitter spawn budget and upload
    //~ the emit table emitter_id is a stable handle so that a slow emitters
    //~ fractional spawn carry can follow it across frames
    struct particle_emitter_desc
    {
        std::uint32_t emitter_id   { 0u };
        std::uint32_t type         { 0u };            //~ :particle_type as a uint dont wanna include heavy
        float         position[3]  { 0.f, 0.f, 0.f };
        float         radius       { 0.f };           //~ spawn sphere radius
        float         min_lifetime { 0.f };
        float         max_lifetime { 0.f };
        float         start_size   { 0.f };
        std::uint32_t base_color   { 0xffffffffu };   //~ packed rgba
        float         spawn_rate   { 0.f };           //~ particles per second
    };

    struct scene_snapshot
    {
        std::uint64_t frame_id  { 0 };
        float         delta_time{ 0.f };

        //~ data
        camera_data   camera{};

        //~ emitters the game wants alive this frame the render side never keeps
        //~ them past the frame so the builder refills the list each time
        std::vector<particle_emitter_desc> particle_emitters{};

        void clear() noexcept
        {
            //~ emitters are rebuilt every frame so wipe them or they pile up
            particle_emitters.clear();
        }
    };
} // namespace trishul::render

#endif //CURSEOFTHESEA_RENDER_SNAPSHOT_H
