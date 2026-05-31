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
#ifndef CURSEOFTHESEA_BVH_H
#define CURSEOFTHESEA_BVH_H

#include <cstdint>
#include <span>
#include <vector>
#include <DirectXMath.h>

#include "frustum.h"

namespace trishul::render::cull
{
    //~ holding one instance as a build primitive the world box plus which
    //~ instance row it maps back to the build copies these it keeps no pointer
    struct bvh_primitive
    {
        DirectX::XMFLOAT3 aabb_min       { 0.f, 0.f, 0.f };
        DirectX::XMFLOAT3 aabb_max       { 0.f, 0.f, 0.f };
        std::uint32_t     instance_index { 0u };
    };

    //~ packing a node flat count zero means internal and first names the left
    //~ child right sits at first plus one since the build keeps siblings next
    //~ to each other count above zero means leaf and first is the offset into
    //~ prim_indices
    struct bvh_node
    {
        DirectX::XMFLOAT3 aabb_min { 0.f, 0.f, 0.f };
        DirectX::XMFLOAT3 aabb_max { 0.f, 0.f, 0.f };
        std::uint32_t     first    { 0u };
        std::uint32_t     count    { 0u };
    };

    //~ classifying a box against the frustum inside lets the walk skip a whole
    //~ subtree intersect makes it keep descending outside prunes it
    enum class frustum_class : std::uint8_t
    {
        outside   = 0,
        intersect = 1,
        inside    = 2,
    };

    [[nodiscard]] frustum_class classify_aabb(
        const frustum& f,
        const DirectX::XMFLOAT3& world_min,
        const DirectX::XMFLOAT3& world_max) noexcept;

    //~ deriving a world box from a local one always off the local box never a
    //~ previous world box so refit cannot bloat a rotated instance over frames
    void world_aabb_from_local(
        const DirectX::XMFLOAT3& local_min,
        const DirectX::XMFLOAT3& local_max,
        const DirectX::XMFLOAT4X4& world,
        DirectX::XMFLOAT3& out_min,
        DirectX::XMFLOAT3& out_max) noexcept;

    //~ a binned sah bvh over per instance world boxes build remakes topology
    //~ and boxes from scratch refit keeps the topology and just walks the boxes
    //~ up traverse_frustum walks it emitting the visible instance ids
    class bvh final
    {
    public:
        //~ keeping leaves small four matches what pbrt and embree ship around
        static constexpr std::uint32_t k_leaf_size = 4u;
        //~ binning into twelve buckets enough to find a good split cheaply
        static constexpr std::uint32_t k_bin_count = 12u;

        bvh() = default;

        void clear() noexcept;

        //~ building topology and boxes fresh dropping any prior state the prims
        //~ span only needs to outlive the call we copy what we need
        void build(std::span<const bvh_primitive> prims);

        //~ refitting the existing topology to the latest boxes walking bottom up
        //~ the prims must be the same set the build saw a size mismatch just
        //~ bails so the caller rebuilds when the instance set actually changes
        void refit(std::span<const bvh_primitive> prims);

        //~ walking the tree depth first inside short circuits a subtree
        //~ intersect keeps recursing leaves emit every instance they own no per
        //~ item test since the node box already cleared them out_visible comes
        //~ back in depth first order not sorted clears on entry
        void traverse_frustum(const frustum& f,
                              std::vector<std::uint32_t>& out_visible) const;

        [[nodiscard]] std::size_t node_count() const noexcept { return nodes_.size(); }
        [[nodiscard]] std::size_t leaf_count() const noexcept { return leaf_count_; }
        [[nodiscard]] std::size_t prim_count() const noexcept { return prim_indices_.size(); }
        [[nodiscard]] bool        empty()      const noexcept { return prim_indices_.empty(); }

        //~ reporting what the last walk touched the renderer grabs these right
        //~ after a traverse to publish into cull_stats reset every traverse
        [[nodiscard]] std::uint32_t last_traversal_node_tests() const noexcept
        {
            return last_node_tests_;
        }
        [[nodiscard]] std::uint32_t last_traversal_leaf_emits() const noexcept
        {
            return last_leaf_emits_;
        }

    private:
        void build_recursive(std::uint32_t node_idx,
                             std::uint32_t start,
                             std::uint32_t end,
                             std::span<const bvh_primitive> prims,
                             std::span<const DirectX::XMFLOAT3> centroids);
        void refit_recursive(std::uint32_t node_idx,
                             std::span<const bvh_primitive> prims);

    private:
        std::vector<bvh_node>      nodes_;
        std::vector<std::uint32_t> prim_indices_;

        //~ mapping a primitive array slot to the instance id the caller wants
        //~ filled at build so traverse can hand back real instance ids without
        //~ keeping the prims span around
        std::vector<std::uint32_t> instance_ids_;
        std::size_t                leaf_count_      { 0 };

        //~ last walk counters mutable since traverse is const the renderer only
        //~ reads them right after on the same thread that walked
        mutable std::uint32_t      last_node_tests_ { 0u };
        mutable std::uint32_t      last_leaf_emits_ { 0u };
    };
} // namespace trishul::render::cull

#endif //CURSEOFTHESEA_BVH_H
