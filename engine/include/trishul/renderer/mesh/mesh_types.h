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
#ifndef CURSEOFTHESEA_MESH_TYPES_H
#define CURSEOFTHESEA_MESH_TYPES_H

#include <cfloat>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <DirectXMath.h>

#include "trishul/core/slot_map.h"

namespace trishul::render::mesh
{
    //~ local space axis aligned box every mesh and submesh carries one bvh
    //~ frustum cull
    struct aabb
    {
        DirectX::XMFLOAT3 min{  FLT_MAX,  FLT_MAX,  FLT_MAX };
        DirectX::XMFLOAT3 max{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

        void grow(const DirectX::XMFLOAT3& p) noexcept
        {
            min.x = p.x < min.x ? p.x : min.x;
            min.y = p.y < min.y ? p.y : min.y;
            min.z = p.z < min.z ? p.z : min.z;
            max.x = p.x > max.x ? p.x : max.x;
            max.y = p.y > max.y ? p.y : max.y;
            max.z = p.z > max.z ? p.z : max.z;
        }

        [[nodiscard]] bool valid() const noexcept
        {
            return max.x >= min.x && max.y >= min.y && max.z >= min.z;
        }

        [[nodiscard]] DirectX::XMFLOAT3 center() const noexcept
        {
            return { (min.x + max.x) * 0.5f,
                     (min.y + max.y) * 0.5f,
                     (min.z + max.z) * 0.5f };
        }

        [[nodiscard]] DirectX::XMFLOAT3 half_extent() const noexcept
        {
            return { (max.x - min.x) * 0.5f,
                     (max.y - min.y) * 0.5f,
                     (max.z - min.z) * 0.5f };
        }
    };

    //~ cheap sphere reject
    struct bounding_sphere
    {
        DirectX::XMFLOAT3 center{ 0.f, 0.f, 0.f };
        float             radius{ 0.f };
    };

    //~ which vertex streams a mesh actually carries baked into the cooked header
    //~ so the loader knows what to read back skinned gates the joints and weights
    enum class mesh_format : std::uint32_t
    {
        none      = 0u,
        positions = 1u << 0,
        normals   = 1u << 1,
        uvs       = 1u << 2,
        colors    = 1u << 3,
        tangents  = 1u << 4,
        skinned   = 1u << 5,
    };

    [[nodiscard]] constexpr mesh_format operator|(const mesh_format a, const mesh_format b) noexcept
    {
        return static_cast<mesh_format>(
            static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
    }
    constexpr mesh_format& operator|=(mesh_format& a, const mesh_format b) noexcept
    {
        a = a | b;
        return a;
    }
    [[nodiscard]] constexpr bool has_flag(const mesh_format set, const mesh_format flag) noexcept
    {
        return (static_cast<std::uint32_t>(set) & static_cast<std::uint32_t>(flag)) != 0u;
    }

    //~ four bone influences per vertex the standard cap reserved skinning only
    struct joint_indices
    {
        std::uint16_t j[4]{ 0u, 0u, 0u, 0u };
    };

    //~ a contiguous slice of the index buffer sharing one material the graph
    //~ later issues one draw per submesh
    struct submesh
    {
        std::uint32_t index_offset { 0u };
        std::uint32_t index_count  { 0u };
        std::uint32_t material_slot{ 0u };
        aabb          bounds       {};
    };

    //~ reserved skeleton bind pose
    struct bone
    {
        std::string         name;
        std::int32_t        parent{ -1 };  //~ index into bones or negative for root
        DirectX::XMFLOAT4X4 inverse_bind{};
    };
    struct skeleton
    {
        std::vector<bone> bones;
    };

    //~ the cpu side mesh asset structure of arrays
    struct mesh_data
    {
        std::vector<DirectX::XMFLOAT3> positions;
        std::vector<DirectX::XMFLOAT3> normals;
        std::vector<DirectX::XMFLOAT2> uvs;
        std::vector<DirectX::XMFLOAT3> colors;
        std::vector<DirectX::XMFLOAT4> tangents;

        //~ skinned only populated when format carries skinned reserveing for now
        std::vector<joint_indices>     joints;
        std::vector<DirectX::XMFLOAT4> weights;

        std::vector<std::uint32_t>     indices;
        std::vector<submesh>           submeshes;

        std::optional<skeleton>        skin;  //~ reserved bind pose

        mesh_format     format{ mesh_format::none };
        aabb            bounds{};
        bounding_sphere sphere{};
        std::string     name;

        [[nodiscard]] std::uint32_t vertex_count() const noexcept
        {
            return static_cast<std::uint32_t>(positions.size());
        }
        [[nodiscard]] std::uint32_t index_count() const noexcept
        {
            return static_cast<std::uint32_t>(indices.size());
        }

        //~ rough cpu footprint for the stats overlay just the stream blobs
        [[nodiscard]] std::uint64_t cpu_bytes() const noexcept
        {
            return positions.size() * sizeof(DirectX::XMFLOAT3)
                 + normals  .size() * sizeof(DirectX::XMFLOAT3)
                 + uvs      .size() * sizeof(DirectX::XMFLOAT2)
                 + colors   .size() * sizeof(DirectX::XMFLOAT3)
                 + tangents .size() * sizeof(DirectX::XMFLOAT4)
                 + joints   .size() * sizeof(joint_indices)
                 + weights  .size() * sizeof(DirectX::XMFLOAT4)
                 + indices  .size() * sizeof(std::uint32_t);
        }

        //~ walk positions for the whole mesh box then per submesh from its index
        //~ slice also drops the bounding sphere call after the streams are filled
        void recompute_bounds() noexcept;

        //~ basic sanity positions present and every index lands inside them
        [[nodiscard]] bool valid() const noexcept;
    };

    //~ stable id for the editor and the registry
    struct mesh_tag {};
    using mesh_handle = handle<mesh_tag>;

    //~ one placed mesh in the world editor foundation only for now
    struct mesh_instance_desc
    {
        mesh_handle         mesh       {};
        DirectX::XMFLOAT4X4 world      {};
        std::uint32_t       material_id{ 0u };
        std::uint64_t       stable_id  { 0u };  //~ editor selection identity
    };
} // namespace trishul::render::mesh

#endif //CURSEOFTHESEA_MESH_TYPES_H
