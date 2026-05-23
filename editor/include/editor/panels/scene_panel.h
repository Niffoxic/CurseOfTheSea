// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_EDITOR_SCENE_PANEL_H
#define CURSEOFTHESEA_EDITOR_SCENE_PANEL_H

#include "editor/panel.h"

namespace cots::editor
{
    //~ lists editor owned actors and exposes spawn delete buttons
    class scene_panel final : public panel
    {
    public:
        void draw() override;

        [[nodiscard]] const char* name() const noexcept override
        {
            return "scene";
        }

    private:
        bool open_{ true };
        int  spawn_mesh_index_   { 0 };
        int  spawn_material_index_{ 0 };
        float spawn_position_[3] { 0.f, 0.f, 0.f };
    };
} // namespace cots::editor

#endif //CURSEOFTHESEA_EDITOR_SCENE_PANEL_H
