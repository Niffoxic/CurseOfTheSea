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
#include "trishul/renderer/graph/resource_registry.h"
#include "trishul/core/engine_assert.h"

namespace trishul::render::graph
{
    namespace
    {
        constexpr resource_view k_empty_view{};
    } // namespace anonymous

    resource_handle resource_registry::import(
        const char       *debug_name,
        resource_usage    initial_usage,
        resource_provider provider,
        bool              preserve_contents)
    {

    }

    void resource_registry::refresh()
    {

    }

    void resource_registry::clear()
    {

    }

    const resource_view & resource_registry::view(resource_handle h) const
    {

    }

    const char * resource_registry::debug_name(resource_handle h) const
    {

    }

    resource_usage resource_registry::initial_usage(resource_handle h) const
    {

    }

    bool resource_registry::preserve_contents(resource_handle h) const
    {

    }

    bool resource_registry::exists(resource_handle h) const noexcept
    {

    }

    std::uint32_t resource_registry::size() const noexcept
    {

    }

    const std::vector<resource_handle> & resource_registry::imports() const noexcept
    {

    }
} // namespace trishul::render::graph
