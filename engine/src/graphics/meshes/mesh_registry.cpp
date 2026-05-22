// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/meshes/mesh_registry.h"
#include "engine/graphics/hardware/buffer_manager.h"

#include <spdlog/spdlog.h>

namespace cots::graphics::meshes
{
    bool mesh_registry::initialize(hardware::buffer_manager& bm)
    {
        bm_ = &bm;
        meshes_.reserve(16);
        return true;
    }

    void mesh_registry::deinitialize()
    {
        meshes_.clear();
        bm_ = nullptr;
    }

    mesh_id mesh_registry::create(const mesh_desc& desc)
    {
        if (!bm_) return invalid_mesh;

        mesh m;
        m.vertex_count = desc.vertex_count;
        m.streams.reserve(desc.streams.size());

        for (const auto& s : desc.streams)
        {
            hardware::buffer_create_info bi{};
            bi.size_bytes   = s.size_bytes;
            bi.kind         = hardware::buffer_kind::vertex;
            bi.initial_data = s.data;
            bi.stride       = s.stride;
            bi.debug_name   = s.semantic.c_str();

            const auto h = bm_->create(bi);
            if (!h.valid())
            {
                spdlog::error("[mesh] stream '{}' alloc failed", s.semantic);
                return invalid_mesh;
            }
            m.streams.push_back({ s.semantic, h, s.stride });
        }

        if (desc.index_data && desc.index_count)
        {
            hardware::buffer_create_info bi{};
            bi.size_bytes   = desc.index_bytes;
            bi.kind         = hardware::buffer_kind::index;
            bi.initial_data = desc.index_data;
            bi.debug_name   = "index";

            const auto h = bm_->create(bi);
            if (!h.valid())
            {
                spdlog::error("[mesh] index alloc failed");
                return invalid_mesh;
            }
            m.index       = h;
            m.index_count = desc.index_count;
            m.index_16bit = desc.index_16bit;
        }

        meshes_.push_back(std::move(m));
        return static_cast<mesh_id>(meshes_.size() - 1);
    }

    const mesh* mesh_registry::get(const mesh_id id) const
    {
        return id < meshes_.size() ? &meshes_[id] : nullptr;
    }
} // namespace cots::graphics::meshes
