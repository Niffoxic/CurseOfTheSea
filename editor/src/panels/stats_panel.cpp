// Created by Niffoxic (Harsh Dubey)
#include "editor/panels/stats_panel.h"
#include "editor/command.h"
#include "editor/editor.h"

#include "engine/graphics/render.h"
#include "engine/system/feature_locator.h"

#include <imgui.h>

namespace cots::editor
{
    void stats_panel::draw()
    {
        if (!open_) return;
        if (!ImGui::Begin("stats", &open_))
        {
            ImGui::End();
            return;
        }

        const auto r = cots::feature::locator::resolve<cots::graphics::render>();
        if (r)
        {
            ImGui::Text("rt fps    : %.1f", r->fps());
            ImGui::Text("rt ms     : %.2f", r->frame_ms());
            ImGui::Text("rt ready  : %s",   r->is_ready() ? "yes" : "no");
        }
        else
        {
            ImGui::Text("render subsystem unavailable");
        }

        ImGui::Separator();
        ImGui::Text("undo depth: %zu", undo_depth());
        ImGui::Text("redo depth: %zu", redo_depth());
        if (ImGui::Button("undo")) undo();
        ImGui::SameLine();
        if (ImGui::Button("redo")) redo();
        ImGui::SameLine();
        if (ImGui::Button("clear history")) clear_command_history();

        ImGui::End();
    }
} // namespace cots::editor
