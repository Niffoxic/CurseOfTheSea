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
#include "trishul/renderer/cull/bvh.h"

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <utility>

namespace trishul::render::cull
{
    namespace
    {
        //~ starting an empty box at the extremes so the first union just takes
        //~ over no numeric_limits pull in for one pair of constants
        constexpr DirectX::XMFLOAT3 k_empty_min { FLT_MAX, FLT_MAX, FLT_MAX };
        constexpr DirectX::XMFLOAT3 k_empty_max { -FLT_MAX, -FLT_MAX, -FLT_MAX };

        inline DirectX::XMFLOAT3 elemwise_min(const DirectX::XMFLOAT3& a,
                                              const DirectX::XMFLOAT3& b) noexcept
        {
            return { std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z) };
        }

        inline DirectX::XMFLOAT3 elemwise_max(const DirectX::XMFLOAT3& a,
                                              const DirectX::XMFLOAT3& b) noexcept
        {
            return { std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z) };
        }

        //~ measuring surface area for the sah cost clamping each side to non
        //~ negative so a collapsed slab still gives a sane number
        inline float surface_area(const DirectX::XMFLOAT3& mn,
                                  const DirectX::XMFLOAT3& mx) noexcept
        {
            const float dx = std::max(0.f, mx.x - mn.x);
            const float dy = std::max(0.f, mx.y - mn.y);
            const float dz = std::max(0.f, mx.z - mn.z);
            return 2.f * (dx * dy + dy * dz + dz * dx);
        }

        //~ indexing a float3 by axis keeping the binning loop switch free
        inline float axis_at(const DirectX::XMFLOAT3& v, int axis) noexcept
        {
            return (axis == 0) ? v.x : (axis == 1) ? v.y : v.z;
        }
    } //~ anonymous namespace

    frustum_class classify_aabb(const frustum& f,
                                const DirectX::XMFLOAT3& world_min,
                                const DirectX::XMFLOAT3& world_max) noexcept
    {
        //~ taking centre plus extent then checking each plane if the box sits
        //~ fully behind one it is outside if even its nearest corner clears
        //~ every plane it is inside anything else straddles so intersect
        const DirectX::XMFLOAT3 centre
        {
            0.5f * (world_min.x + world_max.x),
            0.5f * (world_min.y + world_max.y),
            0.5f * (world_min.z + world_max.z),
        };
        const DirectX::XMFLOAT3 extent
        {
            0.5f * (world_max.x - world_min.x),
            0.5f * (world_max.y - world_min.y),
            0.5f * (world_max.z - world_min.z),
        };

        bool fully_inside = true;
        for (const auto& p : f.planes)
        {
            const float dist = centre.x * p.normal.x
                             + centre.y * p.normal.y
                             + centre.z * p.normal.z + p.d;
            const float r = extent.x * std::fabs(p.normal.x)
                          + extent.y * std::fabs(p.normal.y)
                          + extent.z * std::fabs(p.normal.z);
            if (dist + r < 0.f) return frustum_class::outside;
            if (dist - r < 0.f) fully_inside = false;
        }
        return fully_inside ? frustum_class::inside : frustum_class::intersect;
    }

    void world_aabb_from_local(const DirectX::XMFLOAT3& local_min,
                               const DirectX::XMFLOAT3& local_max,
                               const DirectX::XMFLOAT4X4& world,
                               DirectX::XMFLOAT3& out_min,
                               DirectX::XMFLOAT3& out_max) noexcept
    {
        //~ folding the world transform into a local box the centre rides the
        //~ full matrix the extent picks up the abs of the upper left 3x3 same
        //~ math as the frustum test so the two agree on one world box
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

        out_min.x = world_centre.x - world_extent.x;
        out_min.y = world_centre.y - world_extent.y;
        out_min.z = world_centre.z - world_extent.z;
        out_max.x = world_centre.x + world_extent.x;
        out_max.y = world_centre.y + world_extent.y;
        out_max.z = world_centre.z + world_extent.z;
    }

    //~ bvh
    void bvh::clear() noexcept
    {
        nodes_       .clear();
        prim_indices_.clear();
        instance_ids_.clear();
        leaf_count_      = 0;
        last_node_tests_ = 0u;
        last_leaf_emits_ = 0u;
    }

    void bvh::build(std::span<const bvh_primitive> prims)
    {
        nodes_       .clear();
        prim_indices_.clear();
        instance_ids_.clear();
        leaf_count_  = 0;

        if (prims.empty()) return;

        //~ laying down an identity permutation the build sorts into it in place
        //~ and snapshotting each instance id alongside so traverse can hand back
        //~ real ids later without keeping the prims span around
        prim_indices_.resize(prims.size());
        instance_ids_.resize(prims.size());
        for (std::uint32_t i = 0; i < prims.size(); ++i)
        {
            prim_indices_[i] = i;
            instance_ids_[i] = prims[i].instance_index;
        }

        //~ caching centroids once binning reads them per node so paying for them
        //~ up front beats recomputing off the boxes every descent
        std::vector<DirectX::XMFLOAT3> centroids(prims.size());
        for (std::size_t i = 0; i < prims.size(); ++i)
        {
            centroids[i].x = 0.5f * (prims[i].aabb_min.x + prims[i].aabb_max.x);
            centroids[i].y = 0.5f * (prims[i].aabb_min.y + prims[i].aabb_max.y);
            centroids[i].z = 0.5f * (prims[i].aabb_min.z + prims[i].aabb_max.z);
        }

        nodes_.reserve(prims.size() * 2u);
        nodes_.emplace_back(); //~ root at index zero

        build_recursive(0u,
                        0u,
                        static_cast<std::uint32_t>(prims.size()),
                        prims,
                        centroids);
    }

    void bvh::build_recursive(const std::uint32_t node_idx,
                              const std::uint32_t start,
                              const std::uint32_t end,
                              const std::span<const bvh_primitive> prims,
                              const std::span<const DirectX::XMFLOAT3> centroids)
    {
        //~ growing the node box and centroid bounds over the range indexing
        //~ nodes_ never holding a ref since pushing children can move the vector
        DirectX::XMFLOAT3 node_min = k_empty_min;
        DirectX::XMFLOAT3 node_max = k_empty_max;
        DirectX::XMFLOAT3 cb_min   = k_empty_min;
        DirectX::XMFLOAT3 cb_max   = k_empty_max;
        for (std::uint32_t i = start; i < end; ++i)
        {
            const std::uint32_t pi = prim_indices_[i];
            node_min = elemwise_min(node_min, prims[pi].aabb_min);
            node_max = elemwise_max(node_max, prims[pi].aabb_max);
            cb_min   = elemwise_min(cb_min,   centroids[pi]);
            cb_max   = elemwise_max(cb_max,   centroids[pi]);
        }
        nodes_[node_idx].aabb_min = node_min;
        nodes_[node_idx].aabb_max = node_max;

        const std::uint32_t n = end - start;
        if (n <= k_leaf_size)
        {
            nodes_[node_idx].first = start;
            nodes_[node_idx].count = n;
            ++leaf_count_;
            return;
        }

        //~ splitting along the fattest centroid axis if every centroid lands on
        //~ the same spot there fall through to a leaf below
        const DirectX::XMFLOAT3 cb_extent
        {
            cb_max.x - cb_min.x,
            cb_max.y - cb_min.y,
            cb_max.z - cb_min.z,
        };
        int axis = 0;
        if (cb_extent.y > cb_extent.x) axis = 1;
        if (axis_at(cb_extent, 2) > axis_at(cb_extent, axis)) axis = 2;

        const float axis_extent = axis_at(cb_extent, axis);
        if (axis_extent < 1e-6f)
        {
            nodes_[node_idx].first = start;
            nodes_[node_idx].count = n;
            ++leaf_count_;
            return;
        }

        //~ dropping each centroid into one of twelve bins along the axis the one
        //~ minus epsilon factor keeps a right edge centroid out of bin twelve
        //~ which would run past the array
        struct bin
        {
            std::uint32_t       count = 0u;
            DirectX::XMFLOAT3   min   = k_empty_min;
            DirectX::XMFLOAT3   max   = k_empty_max;
        };
        std::array<bin, k_bin_count> bins{};

        const float k1 = static_cast<float>(k_bin_count) * (1.f - 1e-6f) / axis_extent;
        const float cb_min_axis = axis_at(cb_min, axis);
        for (std::uint32_t i = start; i < end; ++i)
        {
            const std::uint32_t pi = prim_indices_[i];
            const float c = axis_at(centroids[pi], axis);
            int b_idx = static_cast<int>((c - cb_min_axis) * k1);
            if (b_idx < 0) b_idx = 0;
            if (b_idx >= static_cast<int>(k_bin_count)) b_idx = static_cast<int>(k_bin_count) - 1;

            bins[b_idx].count += 1u;
            bins[b_idx].min   = elemwise_min(bins[b_idx].min, prims[pi].aabb_min);
            bins[b_idx].max   = elemwise_max(bins[b_idx].max, prims[pi].aabb_max);
        }

        //~ sweeping left to right then right to left so each split plane knows
        //~ the bounds and count on either side trav and isect cost both sit at
        //~ one here only the relative scale matters
        constexpr std::uint32_t k_splits = k_bin_count - 1u;
        std::array<DirectX::XMFLOAT3, k_splits> left_min{};
        std::array<DirectX::XMFLOAT3, k_splits> left_max{};
        std::array<DirectX::XMFLOAT3, k_splits> right_min{};
        std::array<DirectX::XMFLOAT3, k_splits> right_max{};
        std::array<std::uint32_t,     k_splits> left_count{};
        std::array<std::uint32_t,     k_splits> right_count{};

        {
            DirectX::XMFLOAT3 acc_min = k_empty_min;
            DirectX::XMFLOAT3 acc_max = k_empty_max;
            std::uint32_t acc_count = 0u;
            for (std::uint32_t i = 0; i < k_splits; ++i)
            {
                acc_count += bins[i].count;
                acc_min    = elemwise_min(acc_min, bins[i].min);
                acc_max    = elemwise_max(acc_max, bins[i].max);
                left_count[i] = acc_count;
                left_min[i]   = acc_min;
                left_max[i]   = acc_max;
            }
        }
        {
            DirectX::XMFLOAT3 acc_min = k_empty_min;
            DirectX::XMFLOAT3 acc_max = k_empty_max;
            std::uint32_t acc_count = 0u;
            for (std::uint32_t i = k_bin_count - 1u; i > 0; --i)
            {
                acc_count += bins[i].count;
                acc_min    = elemwise_min(acc_min, bins[i].min);
                acc_max    = elemwise_max(acc_max, bins[i].max);
                right_count[i - 1u] = acc_count;
                right_min[i - 1u]   = acc_min;
                right_max[i - 1u]   = acc_max;
            }
        }

        const float parent_area = surface_area(node_min, node_max);
        const float inv_parent  = parent_area > 0.f ? 1.f / parent_area : 0.f;
        constexpr float t_trav  = 1.0f;
        constexpr float t_isect = 1.0f;
        const float leaf_cost   = static_cast<float>(n) * t_isect;

        //~ picking the cheapest split skipping any with an empty side
        int   best_split = -1;
        float best_cost  = FLT_MAX;
        for (std::uint32_t i = 0; i < k_splits; ++i)
        {
            if (left_count[i] == 0u || right_count[i] == 0u) continue;
            const float la = surface_area(left_min[i],  left_max[i]);
            const float ra = surface_area(right_min[i], right_max[i]);
            const float cost = t_trav
                + (la * static_cast<float>(left_count[i])
                +  ra * static_cast<float>(right_count[i])) * inv_parent * t_isect;
            if (cost < best_cost)
            {
                best_cost  = cost;
                best_split = static_cast<int>(i);
            }
        }

        //~ making a leaf when nothing splits or splitting costs more than just
        //~ paying for the whole range as one leaf
        if (best_split < 0 || best_cost >= leaf_cost)
        {
            nodes_[node_idx].first = start;
            nodes_[node_idx].count = n;
            ++leaf_count_;
            return;
        }

        //~ partitioning prim_indices around the chosen split by recomputing each
        //~ centroid bin the returned iterator marks the start of the right block
        const auto begin_it = prim_indices_.begin() + start;
        const auto end_it   = prim_indices_.begin() + end;
        const auto mid_it = std::partition(begin_it, end_it,
            [&](std::uint32_t pi)
            {
                const float c = axis_at(centroids[pi], axis);
                int b_idx = static_cast<int>((c - cb_min_axis) * k1);
                if (b_idx < 0) b_idx = 0;
                if (b_idx >= static_cast<int>(k_bin_count)) b_idx = static_cast<int>(k_bin_count) - 1;
                return b_idx <= best_split;
            });
        std::uint32_t mid = static_cast<std::uint32_t>(
            std::distance(prim_indices_.begin(), mid_it));

        //~ falling back to a median split if the binning rounded everything onto
        //~ one side so the recursion always makes progress
        if (mid == start || mid == end)
        {
            mid = start + n / 2u;
        }

        //~ allocating the two children side by side storing the left index then
        //~ zeroing count to mark internal not holding a node ref across the push
        const std::uint32_t left = static_cast<std::uint32_t>(nodes_.size());
        nodes_.emplace_back();
        nodes_.emplace_back();
        nodes_[node_idx].first = left;
        nodes_[node_idx].count = 0u;

        build_recursive(left,       start, mid, prims, centroids);
        build_recursive(left + 1u,  mid,   end, prims, centroids);
    }

    void bvh::refit(const std::span<const bvh_primitive> prims)
    {
        if (nodes_.empty() || prim_indices_.empty()) return;
        //~ keeping the topology and just moving the boxes bailing if the set
        //~ shrank since the leaves still point through the build permutation the
        //~ caller rebuilds when the instance set actually changes
        if (prims.size() < prim_indices_.size()) return;

        refit_recursive(0u, prims);
    }

    void bvh::refit_recursive(const std::uint32_t node_idx,
                              const std::span<const bvh_primitive> prims)
    {
        //~ leaf unions the latest boxes of everything it owns always reading the
        //~ fresh world box never the old node bound so a spinning instance does
        //~ not bloat internal recurses first then unions its two kids
        if (nodes_[node_idx].count > 0u)
        {
            DirectX::XMFLOAT3 mn = k_empty_min;
            DirectX::XMFLOAT3 mx = k_empty_max;
            const std::uint32_t first = nodes_[node_idx].first;
            const std::uint32_t last  = first + nodes_[node_idx].count;
            for (std::uint32_t i = first; i < last; ++i)
            {
                const std::uint32_t pi = prim_indices_[i];
                if (pi >= prims.size()) continue;
                mn = elemwise_min(mn, prims[pi].aabb_min);
                mx = elemwise_max(mx, prims[pi].aabb_max);
            }
            nodes_[node_idx].aabb_min = mn;
            nodes_[node_idx].aabb_max = mx;
            return;
        }

        const std::uint32_t left = nodes_[node_idx].first;
        refit_recursive(left,       prims);
        refit_recursive(left + 1u,  prims);

        nodes_[node_idx].aabb_min =
            elemwise_min(nodes_[left].aabb_min, nodes_[left + 1u].aabb_min);
        nodes_[node_idx].aabb_max =
            elemwise_max(nodes_[left].aabb_max, nodes_[left + 1u].aabb_max);
    }

    void bvh::traverse_frustum(const frustum& f,
                               std::vector<std::uint32_t>& out_visible) const
    {
        out_visible.clear();
        last_node_tests_ = 0u;
        last_leaf_emits_ = 0u;
        if (nodes_.empty()) return;

        //~ walking depth first with an explicit stack each frame carries whether
        //~ the parent was fully inside so its kids can skip the plane test and
        //~ just spill their leaves the fixed 128 slots comfortably bound the
        //~ depth a balanced tree over millions of instances never gets near it
        struct entry
        {
            std::uint32_t idx;
            std::uint8_t  parent_inside;
        };

        std::array<entry, 128u> stack{};
        std::uint32_t sp = 0u;
        stack[sp++] = entry{ 0u, 0u };

        while (sp > 0u)
        {
            const entry e = stack[--sp];
            const auto& node = nodes_[e.idx];

            frustum_class cls = frustum_class::inside;
            if (!e.parent_inside)
            {
                cls = classify_aabb(f, node.aabb_min, node.aabb_max);
                ++last_node_tests_;
                if (cls == frustum_class::outside) continue;
            }

            //~ spilling every instance the leaf owns no per item test the node
            //~ box already cleared the leaf so anything inside survives mapping
            //~ the array slot back to its real instance id on the way out
            if (node.count > 0u)
            {
                const std::uint32_t first = node.first;
                const std::uint32_t last  = first + node.count;
                for (std::uint32_t i = first; i < last; ++i)
                {
                    out_visible.push_back(instance_ids_[prim_indices_[i]]);
                }
                last_leaf_emits_ += node.count;
                continue;
            }

            //~ pushing both kids when the stack has room dropping past the cap
            //~ would silently skip a subtree which the divergence check catches
            //~ next frame so no graceful recovery here
            if (sp + 2u > stack.size()) continue;
            const std::uint8_t pi = (cls == frustum_class::inside) ? 1u : 0u;
            stack[sp++] = entry{ node.first,        pi };
            stack[sp++] = entry{ node.first + 1u,   pi };
        }
    }
} // namespace trishul::render::cull