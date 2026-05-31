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
#ifndef CURSEOFTHESEA_FRUSTUM_H
#define CURSEOFTHESEA_FRUSTUM_H

#include <atomic>
#include <cstdint>
#include <DirectXMath.h>

namespace trishul::render::cull
{
    //~ naming what the bvh did this frame so the stats badge can colour itself
    enum class bvh_built_kind : std::uint32_t
    {
        none    = 0u,
        refit   = 1u,
        rebuild = 2u,
    };

    //~ holding the per frame cull counters the editor panel reads these with
    //~ relaxed loads no locking render owns the storage and does the writing
    struct cull_stats
    {
        std::atomic<std::uint32_t> total   { 0u };
        std::atomic<std::uint32_t> culled  { 0u };
        std::atomic<std::uint32_t> drawn   { 0u };

        //~ counting the bvh side of things node and leaf totals what ran this
        //~ frame how long the walk took and how many candidates fell out
        std::atomic<std::uint32_t> bvh_node_count       { 0u };
        std::atomic<std::uint32_t> bvh_leaf_count       { 0u };
        std::atomic<std::uint32_t> bvh_built_kind       { 0u }; //~ reading as bvh_built_kind
        std::atomic<std::uint32_t> bvh_traversal_us     { 0u };
        std::atomic<std::uint32_t> bvh_candidate_count  { 0u };
        std::atomic<std::uint32_t> bvh_bruteforce_count { 0u };
        std::atomic<std::uint32_t> bvh_node_tests       { 0u };
        std::atomic<std::uint32_t> bvh_leaf_emits       { 0u };
        std::atomic<std::uint32_t> bvh_divergence_count { 0u }; //~ ticking when bvh and brute force disagree

        //~ tracking the gpu cull path visible count needs a one frame readback
        //~ so it stays at zero until that lands
        std::atomic<std::uint32_t> gpu_instance_count   { 0u };
        std::atomic<std::uint32_t> gpu_bucket_count     { 0u };
        std::atomic<std::uint32_t> gpu_visible_count    { 0u };
        std::atomic<std::uint32_t> gpu_draws_issued     { 0u };
    };

    //~ holding one inward facing half plane a point is inside when normal dot p
    //~ plus d is at least zero normal stays unit length after extraction
    struct plane
    {
        DirectX::XMFLOAT3 normal { 0.f, 0.f, 0.f };
        float             d      { 0.f };
    };

    struct frustum
    {
        plane planes[6];   //~ left right bottom top near far
    };

    //~ pulling the six planes out of a row major view projection gribb hartmann
    //~ style pass view times projection works for normal and reversed z alike
    //~ only the near far labels swap which the tests do not care about
    void extract_from_view_proj(const DirectX::XMFLOAT4X4& view_proj,
                                frustum& out) noexcept;

    //~ testing a local box against the frustum after folding the world matrix
    //~ in centre plus extent through the abs of the upper left 3x3 so rotation
    //~ and non uniform scale stay conservative returning true means keep the
    //~ instance straddlers count as inside
    [[nodiscard]] bool aabb_in_frustum(const frustum& f,
                                       const DirectX::XMFLOAT3& local_min,
                                       const DirectX::XMFLOAT3& local_max,
                                       const DirectX::XMFLOAT4X4& world) noexcept;
} // namespace trishul::render::cull

#endif //CURSEOFTHESEA_FRUSTUM_H