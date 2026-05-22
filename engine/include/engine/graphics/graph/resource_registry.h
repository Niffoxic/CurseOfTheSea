// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_GRAPH_RESOURCE_REGISTRY_H
#define CURSEOFTHESEA_GRAPH_RESOURCE_REGISTRY_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "engine/graphics/graph/resource_handle.h"

struct ID3D12Resource2;

namespace cots::graphics::graph
{
    //~ per frame snapshot of an imported resource
    struct resource_view
    {
        ID3D12Resource2* resource   { nullptr };
        std::size_t      view_handle{ 0 };
        std::uint32_t    width      { 0 };
        std::uint32_t    height     { 0 };
    };

    using resource_provider = std::function<resource_view()>;

    class resource_registry final
    {
    public:
         resource_registry() = default;
        ~resource_registry() = default;

        resource_registry           (const resource_registry&) = delete;
        resource_registry& operator=(const resource_registry&) = delete;
        resource_registry           (resource_registry&&)      = delete;
        resource_registry& operator=(resource_registry&&)      = delete;

        //~ register an externally owned resource returns a stable handle
        resource_handle import(const char* debug_name, resource_provider provider);

        //~ reevaluate all providers and cache the views for this frame
        void refresh();
        void clear  ();

        [[nodiscard]] const resource_view& view      (resource_handle h) const;
        [[nodiscard]] const char*          debug_name(resource_handle h) const;
        [[nodiscard]] bool                 exists    (resource_handle h) const noexcept;
        [[nodiscard]] std::uint32_t        size      () const noexcept;

        //~ enumerate every live import
        [[nodiscard]] const std::vector<resource_handle>& imports() const noexcept;

    private:
        struct entry
        {
            std::string       debug_name;
            resource_provider provider;
            resource_view     cached;
            std::uint32_t     generation{ 0 }; //~ 0 means free slot
        };

        std::vector<entry>             entries_;
        std::vector<resource_handle>   imports_;
        std::uint32_t                  next_generation_{ 1 };
    };
} // namespace cots::graphics::graph

#endif //CURSEOFTHESEA_GRAPH_RESOURCE_REGISTRY_H
