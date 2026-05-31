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
#include "trishul/renderer/cull/frustum.h"

#include <cmath>

namespace trishul::render::cull
{
    namespace
    {
        //~ normalizing so the box test reads as a real world distance staying
        //~ safe if a normal ever drifts off unit length skipping a zero plane
        void normalize_plane(plane& p) noexcept
        {
            const float len = std::sqrt(p.normal.x * p.normal.x
                                      + p.normal.y * p.normal.y
                                      + p.normal.z * p.normal.z);
            if (len > 0.f)
            {
                const float inv = 1.0f / len;
                p.normal.x *= inv;
                p.normal.y *= inv;
                p.normal.z *= inv;
                p.d        *= inv;
            }
        }
    } //~ anonymous namespace

    void extract_from_view_proj(const DirectX::XMFLOAT4X4& m,
                                frustum& out) noexcept
    {
        //~ reading the six clip space half planes straight off the matrix
        //~ columns row major so clip equals world times m the add subtract
        //~ pairs drop out of the inside inequalities same table for either
        //~ depth range since the test is always clip xyz inside the w bound

        //~ left   x + w >= 0
        out.planes[0].normal.x = m._14 + m._11;
        out.planes[0].normal.y = m._24 + m._21;
        out.planes[0].normal.z = m._34 + m._31;
        out.planes[0].d        = m._44 + m._41;

        //~ right  w - x >= 0
        out.planes[1].normal.x = m._14 - m._11;
        out.planes[1].normal.y = m._24 - m._21;
        out.planes[1].normal.z = m._34 - m._31;
        out.planes[1].d        = m._44 - m._41;

        //~ bottom y + w >= 0
        out.planes[2].normal.x = m._14 + m._12;
        out.planes[2].normal.y = m._24 + m._22;
        out.planes[2].normal.z = m._34 + m._32;
        out.planes[2].d        = m._44 + m._42;

        //~ top    w - y >= 0
        out.planes[3].normal.x = m._14 - m._12;
        out.planes[3].normal.y = m._24 - m._22;
        out.planes[3].normal.z = m._34 - m._32;
        out.planes[3].d        = m._44 - m._42;

        //~ near   z >= 0  just a label the inside test holds in reversed z too
        out.planes[4].normal.x = m._13;
        out.planes[4].normal.y = m._23;
        out.planes[4].normal.z = m._33;
        out.planes[4].d        = m._43;

        //~ far    w - z >= 0
        out.planes[5].normal.x = m._14 - m._13;
        out.planes[5].normal.y = m._24 - m._23;
        out.planes[5].normal.z = m._34 - m._33;
        out.planes[5].d        = m._44 - m._43;

        for (auto& p : out.planes) normalize_plane(p);
    }

    bool aabb_in_frustum(const frustum& f,
                         const DirectX::XMFLOAT3& local_min,
                         const DirectX::XMFLOAT3& local_max,
                         const DirectX::XMFLOAT4X4& world) noexcept
    {
        //~ taking the local centre and half extent then pushing them into world
        //~ the centre rides the full transform the extent picks up the abs of
        //~ the upper left 3x3 folding any rotation into a conservative bound
        const DirectX::XMFLOAT3 local_centre
        {
            0.5f * (local_min.x + local_max.x),
            0.5f * (local_min.y + local_max.y),
            0.5f * (local_min.z + local_max.z),
        };
        const DirectX::XMFLOAT3 local_extent
        {
            0.5f * (local_max.x - local_min.x),
            0.5f * (local_max.y - local_min.y),
            0.5f * (local_max.z - local_min.z),
        };

        const DirectX::XMFLOAT3 world_centre
        {
            local_centre.x * world._11 + local_centre.y * world._21
                                       + local_centre.z * world._31 + world._41,
            local_centre.x * world._12 + local_centre.y * world._22
                                       + local_centre.z * world._32 + world._42,
            local_centre.x * world._13 + local_centre.y * world._23
                                       + local_centre.z * world._33 + world._43,
        };

        const float a11 = std::fabs(world._11), a12 = std::fabs(world._12), a13 = std::fabs(world._13);
        const float a21 = std::fabs(world._21), a22 = std::fabs(world._22), a23 = std::fabs(world._23);
        const float a31 = std::fabs(world._31), a32 = std::fabs(world._32), a33 = std::fabs(world._33);

        const DirectX::XMFLOAT3 world_extent
        {
            local_extent.x * a11 + local_extent.y * a21 + local_extent.z * a31,
            local_extent.x * a12 + local_extent.y * a22 + local_extent.z * a32,
            local_extent.x * a13 + local_extent.y * a23 + local_extent.z * a33,
        };

        //~ rejecting only when the centre sits further than its projected extent
        //~ behind a plane so anything that even clips the volume stays in
        for (const auto& p : f.planes)
        {
            const float dist = world_centre.x * p.normal.x
                             + world_centre.y * p.normal.y
                             + world_centre.z * p.normal.z + p.d;
            const float r    = world_extent.x  * std::fabs(p.normal.x)
                             + world_extent.y  * std::fabs(p.normal.y)
                             + world_extent.z  * std::fabs(p.normal.z);
            if (dist + r < 0.f) return false;
        }
        return true;
    }
} // namespace trishul::render::cull