// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_IMPORTED_MODEL_H
#define CURSEOFTHESEA_IMPORTED_MODEL_H

#include <cstdint>
#include <string>
#include <vector>

namespace cots::graphics::meshes
{
    //~ one attribute stream
    struct imported_stream
    {
        std::string               semantic;
        std::uint32_t             stride { 0 };
        std::vector<std::uint8_t> bytes;
    };

    //~ format agnostic mesh
    struct imported_model
    {
        std::vector<imported_stream> streams;
        std::vector<std::uint8_t>    indices;      //~ raw index bytes
        std::uint32_t                vertex_count { 0 };
        std::uint32_t                index_count  { 0 };
        bool                         index_16bit  { true };
        std::string                  source_name;

        //~ TODO: material skin skeleton later

        [[nodiscard]] bool valid() const noexcept
        {
            return vertex_count > 0 && !streams.empty();
        }
    };
} // namespace cots::graphics::meshes

#endif //CURSEOFTHESEA_IMPORTED_MODEL_H
