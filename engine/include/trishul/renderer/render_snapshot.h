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

    struct scene_snapshot
    {
        std::uint64_t frame_id  { 0 };
        float         delta_time{ 0.f };

        //~ data
        camera_data   camera{};

        void clear() noexcept
        {

        }
    };
} // namespace trishul::render

#endif //CURSEOFTHESEA_RENDER_SNAPSHOT_H
