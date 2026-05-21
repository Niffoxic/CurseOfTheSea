// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_RENDER_SNAPSHOT_H
#define CURSEOFTHESEA_RENDER_SNAPSHOT_H

#include <cstdint>
#include <vector>
#include <DirectXMath.h>

namespace cots::graphics
{
    struct camera_data
    {
        DirectX::XMFLOAT4X4 view      {};
        DirectX::XMFLOAT4X4 projection {};
        DirectX::XMFLOAT3   position  { 0.f, 0.f, 0.f };
        DirectX::XMFLOAT3   forward   { 0.f, 0.f, -1.f };
        DirectX::XMFLOAT3   up        { 0.f, 1.f, 0.f };
    };

    //~ placeholder for tests
    struct mesh_instance
    {
        DirectX::XMFLOAT4X4 transform{};
        std::uint32_t       mesh_index    { 0 };
        std::uint32_t       material_index{ 0 };
    };

    struct scene_snapshot
    {
        std::uint64_t frame_id   { 0 };
        float         delta_time { 0.f };

        camera_data   camera{};

        std::vector<mesh_instance> instances; //~ empty rn
        // TODO: lights, environment, debug draws - later

        void clear()
        {
            instances.clear();   //~ keeps capacity
            // frame_id / camera overwritten on next build
        }
    };
} // namespace cots::graphics

#endif
