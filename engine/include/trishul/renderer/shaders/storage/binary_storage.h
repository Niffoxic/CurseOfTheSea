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
#ifndef CURSEOFTHESEA_BINARY_STORAGE_H
#define CURSEOFTHESEA_BINARY_STORAGE_H

#include <iosfwd>

#include "trishul/renderer/shaders/shader_storage.h"

namespace trishul::render::shaders
{
    //~ the binary strategy a tight blob format quick to load and cheap to grow
    //~ store_one just tacks the new entry onto the end no full rewrite needed
    class binary_shader_storage final : public shader_storage
    {
    public:
        explicit binary_shader_storage(std::string path)
        : path_(std::move(path))
        {}

        [[nodiscard]] bool load_all (cache_map& out)      override;
        [[nodiscard]] bool store_all(const cache_map& in) override;

        [[nodiscard]] bool store_one(const shader_cache_entry& e, const cache_map&) override;  //~ appends

    private:
        bool write_entry(std::ofstream& f, const shader_cache_entry& e);
        bool ensure_header() const;   //~ lays down the header if the file is missing

    private:
        std::string path_;
    };
} // namespace trishul::render::shaders

#endif //CURSEOFTHESEA_BINARY_STORAGE_H