// Created by Niffoxic (Harsh Dubey)
#include "editor/commands/load_texture.h"

#include "engine/graphics/render.h"
#include "engine/system/feature_locator.h"

#include <spdlog/spdlog.h>

namespace cots::editor
{
    load_texture::load_texture(std::string path) noexcept
        : path_(std::move(path)) {}

    void load_texture::execute()
    {
        const auto r = cots::feature::locator::resolve<cots::graphics::render>();
        if (!r) return;

        const std::string path = path_;
        r->enqueue_editor_command([path]()
        {
            auto r2 = cots::feature::locator::resolve<cots::graphics::render>();
            if (!r2) return;
            const bool ok = r2->runtime_load_texture(path);
            if (!ok)
            {
                spdlog::warn("[editor] runtime texture load failed for {}", path);
            }
            else
            {
                spdlog::info("[editor] runtime texture {} loaded", path);
            }
        });
        loaded_ = true;
    }

    void load_texture::undo()
    {
        if (!loaded_) return;
        spdlog::info("[editor] load_texture undo for {} no eviction yet", path_);
    }
} // namespace cots::editor
