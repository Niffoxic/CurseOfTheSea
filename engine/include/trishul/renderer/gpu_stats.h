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
#ifndef CURSEOFTHESEA_GPU_STATS_H
#define CURSEOFTHESEA_GPU_STATS_H

#include <cstdint>

namespace trishul::render
{
    //~ gpu stats for the live in megabytes
    struct gpu_memory_info
    {
        double local_budget_mb    { 0.0 }; //~ the vram we got from the driver
        double local_usage_mb     { 0.0 }; //~ vram usage from the budget
        double nonlocal_budget_mb { 0.0 }; //~ shared system memory budget
        double nonlocal_usage_mb  { 0.0 }; //~ shared system memory in use

        double allocated_mb       { 0.0 }; //~ sum of live allocations
        double block_mb           { 0.0 }; //~ memory committed in heaps blocks
        std::uint32_t allocation_count{ 0u };
        std::uint32_t block_count     { 0u };
    };

    //~ its staging pool churn
    struct upload_arena_stats
    {
        std::uint64_t free_staging     { 0u }; //~ pooled buffers ready to reuse
        std::uint64_t in_flight_staging{ 0u }; //~ buffers the gpu still reads
        std::uint64_t reused           { 0u }; //~ pool hits over the lifetime
        std::uint64_t allocated        { 0u }; //~ fresh allocations over the lifetime
    };

    //~ safe snapspot
    struct gpu_stats
    {
        gpu_memory_info    memory;
        upload_arena_stats upload;
        std::uint64_t      deferred_pending{ 0u }; //~ resources queued for deferred release
    };
} // namespace trishul::render

#endif //CURSEOFTHESEA_GPU_STATS_H
