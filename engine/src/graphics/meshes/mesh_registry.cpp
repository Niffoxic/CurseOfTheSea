// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/meshes/mesh_registry.h"
#include "engine/graphics/meshes/imported_model.h"
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

        //~ batch streams and index
        const std::size_t total = desc.streams.size() + (desc.index_count ? 1u : 0u);
        std::vector<hardware::buffer_create_info> infos;
        infos.reserve(total);

        for (const auto& s : desc.streams)
        {
            hardware::buffer_create_info bi{};
            bi.size_bytes   = s.size_bytes;
            bi.kind         = hardware::buffer_kind::vertex;
            bi.initial_data = s.data;
            bi.stride       = s.stride;
            bi.debug_name   = s.semantic.c_str();
            infos.push_back(bi);
        }

        if (desc.index_data && desc.index_count)
        {
            hardware::buffer_create_info bi{};
            bi.size_bytes   = desc.index_bytes;
            bi.kind         = hardware::buffer_kind::index;
            bi.initial_data = desc.index_data;
            bi.debug_name   = "index";
            infos.push_back(bi);
        }

        //~ one flush for all
        const auto handles = bm_->create_batch(infos);
        if (handles.size() != infos.size())
        {
            spdlog::error("[mesh] batch upload failed for '{}'", desc.debug_name);
            return invalid_mesh;
        }

        mesh m;
        m.vertex_count = desc.vertex_count;
        m.streams.reserve(desc.streams.size());
        for (std::size_t i = 0; i < desc.streams.size(); ++i)
        {
            m.streams.push_back({ desc.streams[i].semantic,
                                  handles[i],
                                  desc.streams[i].stride });
        }
        if (desc.index_data && desc.index_count)
        {
            m.index       = handles.back();
            m.index_count = desc.index_count;
            m.index_16bit = desc.index_16bit;
        }

        meshes_.push_back(std::move(m));
        return static_cast<mesh_id>(meshes_.size() - 1);
    }

    mesh_id mesh_registry::create_from_imported(const imported_model& im,
                                                const char* debug_name)
    {
        if (!im.valid()) return invalid_mesh;

        //~ wire descs to bytes
        std::vector<mesh_stream_desc> streams;
        streams.reserve(im.streams.size());
        for (const auto& s : im.streams)
        {
            mesh_stream_desc d{};
            d.semantic   = s.semantic;
            d.data       = s.bytes.data();
            d.size_bytes = static_cast<std::uint32_t>(s.bytes.size());
            d.stride     = s.stride;
            streams.push_back(std::move(d));
        }

        mesh_desc desc{};
        desc.streams      = std::move(streams);
        desc.vertex_count = im.vertex_count;
        desc.index_data   = im.indices.empty() ? nullptr : im.indices.data();
        desc.index_bytes  = static_cast<std::uint32_t>(im.indices.size());
        desc.index_count  = im.index_count;
        desc.index_16bit  = im.index_16bit;
        desc.debug_name   = debug_name ? debug_name : "imported";
        return create(desc);
    }

    const mesh* mesh_registry::get(const mesh_id id) const
    {
        return id < meshes_.size() ? &meshes_[id] : nullptr;
    }
} // namespace cots::graphics::meshes
