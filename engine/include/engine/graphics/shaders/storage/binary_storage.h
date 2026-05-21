// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_BINARY_STORAGE_H
#define CURSEOFTHESEA_BINARY_STORAGE_H

#include "engine/graphics/shaders/shader_storage.h"

namespace cots::graphics::shaders
{
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
        bool ensure_header();   //~ creates file with header if missing

    private:
        std::string path_;
    };
} // namespace cots::graphics::shaders

#endif
