// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_MESH_BAKER_H
#define CURSEOFTHESEA_MESH_BAKER_H

#include <cstdint>
#include <vector>

#include "engine/graphics/meshes/imported_model.h"

namespace cots::graphics::meshes
{
    //~ run mesh opt in place
    //~ dedup cache fetch
    [[nodiscard]] bool optimize_in_place(imported_model& m);

    //~ produces the container bytes
    //~ header records and payloads
    [[nodiscard]] bool serialize_mesh(const imported_model& m,
                                      std::uint64_t source_hash,
                                      std::vector<std::uint8_t>& out_blob);

    //~ inverse of serialize_mesh
    [[nodiscard]] bool deserialize_mesh(const void* data,
                                        std::size_t size,
                                        imported_model& out);
} // namespace cots::graphics::meshes

#endif //CURSEOFTHESEA_MESH_BAKER_H
