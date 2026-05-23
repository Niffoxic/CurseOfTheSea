// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_EDITOR_RESOURCE_PANEL_H
#define CURSEOFTHESEA_EDITOR_RESOURCE_PANEL_H

#include "editor/panel.h"

namespace cots::editor
{
    //~ exposes runtime asset loads through the render command channel
    class resource_panel final : public panel
    {
    public:
        void draw() override;

        [[nodiscard]] const char* name() const noexcept override
        {
            return "resources";
        }

    private:
        bool open_{ true };
        char mesh_path_   [256]{ "assets/models/test.glb"        };
        char texture_path_[256]{ "assets/textures/checker.png"   };
        char mesh_label_  [64] { "loaded_mesh"                   };
    };
} // namespace cots::editor

#endif //CURSEOFTHESEA_EDITOR_RESOURCE_PANEL_H
