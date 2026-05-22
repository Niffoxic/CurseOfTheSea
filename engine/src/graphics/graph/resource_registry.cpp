// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/graph/resource_registry.h"
#include "engine/core/cots_assert.h"

namespace cots::graphics::graph
{
    namespace
    {
        constexpr resource_view k_empty_view{};
    } // namespace anonymous

    resource_handle resource_registry::import(const char* debug_name,
                                              const resource_usage initial_usage,
                                              resource_provider provider,
                                              const bool preserve_contents)
    {
        COTS_ASSERT(provider && "resource_registry::import requires a provider");

        const auto index = static_cast<std::uint32_t>(entries_.size());
        const std::uint32_t gen   = next_generation_++;

        entry e{};
        e.debug_name        = debug_name ? debug_name : "<unnamed>";
        e.provider          = std::move(provider);
        e.initial_usage     = initial_usage;
        e.preserve_contents = preserve_contents;
        e.generation        = gen;
        entries_.push_back(std::move(e));

        const resource_handle h{ index, gen };
        imports_.push_back(h);
        return h;
    }

    void resource_registry::refresh()
    {
        for (auto& e : entries_)
        {
            if (e.generation == 0) continue;
            e.cached = e.provider ? e.provider() : resource_view{};
        }
    }

    void resource_registry::clear()
    {
        entries_.clear();
        imports_.clear();
    }

    const resource_view& resource_registry::view(const resource_handle h) const
    {
        if (!exists(h))
            return k_empty_view;
        return entries_[h.index].cached;
    }

    const char* resource_registry::debug_name(const resource_handle h) const
    {
        if (!exists(h))
            return "<invalid>";
        return entries_[h.index].debug_name.c_str();
    }

    resource_usage resource_registry::initial_usage(const resource_handle h) const
    {
        if (!exists(h)) return resource_usage::common;
        return entries_[h.index].initial_usage;
    }

    bool resource_registry::preserve_contents(const resource_handle h) const
    {
        if (!exists(h)) return false;
        return entries_[h.index].preserve_contents;
    }

    bool resource_registry::exists(const resource_handle h) const noexcept
    {
        if (!h.valid())                       return false;
        if (h.index >= entries_.size())       return false;
        return entries_[h.index].generation == h.generation;
    }

    std::uint32_t resource_registry::size() const noexcept
    {
        return static_cast<std::uint32_t>(entries_.size());
    }

    const std::vector<resource_handle>& resource_registry::imports() const noexcept
    {
        return imports_;
    }
} // namespace cots::graphics::graph
