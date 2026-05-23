// Created by Niffoxic (Harsh Dubey)
#include "editor/panels/scene_panel.h"
#include "editor/command.h"
#include "editor/world.h"
#include "editor/commands/spawn_actor.h"
#include "editor/commands/delete_actor.h"

#include <imgui.h>
#include <DirectXMath.h>

namespace cots::editor
{
    namespace
    {
        DirectX::XMFLOAT4X4 make_translation(const float pos[3])
        {
            const DirectX::XMMATRIX m = DirectX::XMMatrixTranslation(pos[0], pos[1], pos[2]);
            DirectX::XMFLOAT4X4 out{};
            DirectX::XMStoreFloat4x4(&out, m);
            return out;
        }
    } // namespace

    void scene_panel::draw()
    {
        if (!open_) return;
        if (!ImGui::Begin("scene", &open_))
        {
            ImGui::End();
            return;
        }

        ImGui::Text("editor actors: %zu", world_instance().alive_count());
        ImGui::Separator();

        //~ spawn controls
        ImGui::InputInt  ("mesh"    , &spawn_mesh_index_);
        ImGui::InputInt  ("material", &spawn_material_index_);
        ImGui::InputFloat3("position", spawn_position_);

        if (ImGui::Button("spawn"))
        {
            actor a{};
            a.mesh_index     = static_cast<std::uint32_t>(spawn_mesh_index_   < 0 ? 0 : spawn_mesh_index_);
            a.material_index = static_cast<std::uint32_t>(spawn_material_index_< 0 ? 0 : spawn_material_index_);
            a.transform      = make_translation(spawn_position_);
            push_command(std::make_unique<spawn_actor>(a));
        }

        ImGui::Separator();
        ImGui::Text("actor list");

        for (const auto& a : world_instance().actors())
        {
            ImGui::PushID(static_cast<int>(a.id));
            const char* state = a.alive ? "alive" : "dead";
            ImGui::Text("id %llu  mesh %u  %s",
                        static_cast<unsigned long long>(a.id),
                        a.mesh_index, state);
            if (a.alive)
            {
                ImGui::SameLine();
                if (ImGui::SmallButton("delete"))
                {
                    push_command(std::make_unique<delete_actor>(a.id));
                }
            }
            ImGui::PopID();
        }

        ImGui::End();
    }
} // namespace cots::editor
