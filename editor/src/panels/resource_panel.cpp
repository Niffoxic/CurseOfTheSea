// Created by Niffoxic (Harsh Dubey)
#include "editor/panels/resource_panel.h"
#include "editor/command.h"
#include "editor/commands/load_mesh.h"
#include "editor/commands/load_texture.h"

#include <imgui.h>

namespace cots::editor
{
    void resource_panel::draw()
    {
        if (!open_) return;
        if (!ImGui::Begin("resources", &open_))
        {
            ImGui::End();
            return;
        }

        ImGui::Text("runtime asset loads");
        ImGui::Separator();

        ImGui::InputText("mesh path" , mesh_path_   , sizeof(mesh_path_   ));
        ImGui::InputText("mesh label", mesh_label_  , sizeof(mesh_label_  ));
        if (ImGui::Button("load mesh"))
        {
            push_command(std::make_unique<load_mesh>(
                std::string(mesh_path_),
                std::string(mesh_label_)));
        }

        ImGui::Separator();

        ImGui::InputText("texture path", texture_path_, sizeof(texture_path_));
        if (ImGui::Button("load texture"))
        {
            push_command(std::make_unique<load_texture>(std::string(texture_path_)));
        }

        ImGui::End();
    }
} // namespace cots::editor
