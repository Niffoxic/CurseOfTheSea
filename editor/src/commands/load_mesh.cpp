// Created by Niffoxic (Harsh Dubey)
#include "editor/commands/load_mesh.h"

#include "engine/graphics/render.h"
#include "engine/system/feature_locator.h"

#include <spdlog/spdlog.h>

namespace cots::editor
{
    load_mesh::load_mesh(std::string path, std::string label) noexcept
        : path_(std::move(path)), label_(std::move(label)) {}

    void load_mesh::execute()
    {
        const auto r = cots::feature::locator::resolve<cots::graphics::render>();
        if (!r) return;

        const std::string path  = path_;
        const std::string label = label_;
        r->enqueue_editor_command([path, label]()
        {
            //~ runs on the render thread
            auto r2 = cots::feature::locator::resolve<cots::graphics::render>();
            if (!r2) return;
            const std::uint32_t id = r2->runtime_load_mesh(path, label);
            if (id == ~0u)
            {
                spdlog::warn("[editor] runtime mesh load failed for {}", path);
            }
            else
            {
                spdlog::info("[editor] runtime mesh {} loaded as id {}", path, id);
            }
        });
    }

    void load_mesh::undo()
    {
        spdlog::info("[editor] load_mesh undo for {} no eviction yet", path_);
    }
} // namespace cots::editor
