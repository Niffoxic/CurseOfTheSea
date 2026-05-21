// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_JSON_STORAGE_H
#define CURSEOFTHESEA_JSON_STORAGE_H

#include "engine/graphics/shaders/shader_storage.h"

namespace cots::graphics::shaders
{
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
            return store_all(full);
        }

    private:
        std::string path_;
    };
} // namespace cots::graphics::shaders

#endif
