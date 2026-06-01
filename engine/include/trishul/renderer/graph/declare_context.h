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
#ifndef CURSEOFTHESEA_DECLARE_CONTEXT_H
#define CURSEOFTHESEA_DECLARE_CONTEXT_H

#include <vector>
#include "trishul/renderer/graph/resource_handle.h"
#include "trishul/renderer/graph/resource_usage.h"

namespace trishul::render::graph
{
    struct resource_access
    {
        resource_handle handle;
        resource_usage  usage;
    };

    class declare_context final
    {
        using accessors = std::vector<resource_access>;
    public:
        declare_context(accessors& reads, accessors& writes) noexcept
        : reads_(reads), writes_(writes)
        {}

        declare_context           (const declare_context&) = delete;
        declare_context& operator=(const declare_context&) = delete;

        void read(const resource_handle h, const resource_usage a) const noexcept
        {
            reads_.push_back({h, a});
        }

        void write(const resource_handle h, const resource_usage u) const noexcept
        {
            writes_.push_back({ h, u });
        }

    private:
        accessors& reads_;
        accessors& writes_;
    };
} // namespace trishul::render::graph

#endif //CURSEOFTHESEA_DECLARE_CONTEXT_H
