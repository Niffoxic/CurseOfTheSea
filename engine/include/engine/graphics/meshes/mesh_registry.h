// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_MESH_REGISTRY_H
#define CURSEOFTHESEA_MESH_REGISTRY_H

#include <cstdint>
#include <string>
#include <vector>

#include "engine/graphics/hardware/resource.h"

namespace cots::graphics::hardware { class buffer_manager; }

namespace cots::graphics::meshes
{
    using     mesh_id               = std::uint32_t;
    constexpr mesh_id invalid_mesh  = ~0u; //~ TODO: Should I create generalized Invalid Index probably better ig

    struct mesh_stream_desc
    {
        std::string   semantic  {};
        const void*   data      { nullptr };
        std::uint32_t size_bytes{ 0 };
        std::uint32_t stride    { 0 };
    };

    struct mesh_desc
    {
        std::vector<mesh_stream_desc> streams;
        std::uint32_t vertex_count { 0 };

        const void*   index_data  { nullptr };
        std::uint32_t index_bytes { 0 };
        std::uint32_t index_count { 0 };
        bool          index_16bit { true };

        const char*   debug_name  { "mesh" };
    };

    struct mesh
    {
        struct stream
        {
            std::string             semantic;
            hardware::buffer_handle buffer;
            std::uint32_t           stride { 0 };
        };

        std::vector<stream>     streams;
        hardware::buffer_handle index       {};
        std::uint32_t           vertex_count{ 0 };
        std::uint32_t           index_count { 0 };
        bool                    index_16bit { true };

        [[nodiscard]]
        const stream* find(const std::string_view semantic) const
        {
            for (const auto& s : streams)
                if (s.semantic == semantic)
                    return &s;
            return nullptr;
        }
    };

    class mesh_registry final
    {
    public:
        [[nodiscard]]
        bool initialize  (hardware::buffer_manager& bm);
        void deinitialize();

        [[nodiscard]] mesh_id       create(const mesh_desc& desc);
        [[nodiscard]] const mesh*   get   (mesh_id id) const;
        [[nodiscard]] std::uint32_t size  () const noexcept
        {
            return static_cast<std::uint32_t>(meshes_.size());
        }

    private:
        hardware::buffer_manager* bm_{ nullptr };
        std::vector<mesh>         meshes_;
    };
} // namespace cots::graphics::meshes

#endif //CURSEOFTHESEA_MESH_REGISTRY_H
