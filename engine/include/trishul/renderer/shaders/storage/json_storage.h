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
#ifndef CURSEOFTHESEA_JSON_STORAGE_H
#define CURSEOFTHESEA_JSON_STORAGE_H

#include "trishul/renderer/shaders/shader_storage.h"

namespace trishul::render::shaders
{
    //~ the json strategy human readable so you can eyeball the cache store_one
    //~ just rewrites the whole file since json has no cheap append
    class json_shader_storage final : public shader_storage
    {
    public:
        explicit json_shader_storage(std::string path)
        : path_(std::move(path))
        {}

        [[nodiscard]] bool load_all (cache_map& out)      override;
        [[nodiscard]] bool store_all(const cache_map& in) override;

        [[nodiscard]] bool store_one(const shader_cache_entry&, const cache_map& full) override
        {
            return store_all(full);  //~ no partial write so just flush the lot
        }

    private:
        std::string path_;
    };
} // namespace trishul::render::shaders

#endif //CURSEOFTHESEA_JSON_STORAGE_H