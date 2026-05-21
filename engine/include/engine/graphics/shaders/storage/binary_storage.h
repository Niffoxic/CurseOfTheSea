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
    private:
        std::string path_;
    };
} // namespace cots::graphics::shaders

#endif
