// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_MESH_CACHE_H
#define CURSEOFTHESEA_MESH_CACHE_H

#include <memory>
#include <string>
#include <string_view>

#include "engine/graphics/meshes/imported_model.h"
#include "engine/graphics/meshes/model_importer.h"

namespace cots::graphics::meshes
{
    //~ front door for baked meshes
    class mesh_cache final
    {
    public:
        mesh_cache() = default;
        ~mesh_cache() = default;

        mesh_cache           (const mesh_cache&) = delete;
        mesh_cache& operator=(const mesh_cache&) = delete;

        //~ takes the importer
        [[nodiscard]] bool initialize  (std::unique_ptr<model_importer> imp);
        void deinitialize() noexcept;

        //~ load or bake on miss
        [[nodiscard]] bool get_or_bake(std::string_view source_path,
                                       imported_model& out) const;

        //~ wipe the disk container
        void invalidate(std::string_view source_path) const;

    private:
        [[nodiscard]] static std::string baked_path_for(std::string_view source_path);

        [[nodiscard]] bool try_load_cached(const std::string& baked_path,
                                           std::uint64_t      expected_hash,
                                           imported_model&    out) const;

        [[nodiscard]] bool write_container(const std::string& baked_path,
                                           const std::vector<std::uint8_t>& blob) const;

    private:
        std::unique_ptr<model_importer> importer_;
    };
} // namespace cots::graphics::meshes

#endif //CURSEOFTHESEA_MESH_CACHE_H
