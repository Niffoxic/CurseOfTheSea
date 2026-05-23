// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_EDITOR_STATS_PANEL_H
#define CURSEOFTHESEA_EDITOR_STATS_PANEL_H

#include "editor/panel.h"

namespace cots::editor
{
    //~ shows render thread fps and frame ms plus mt counters
    class stats_panel final : public panel
    {
    public:
        void draw() override;

        [[nodiscard]] const char* name() const noexcept override
        {
            return "stats";
        }

    private:
        bool open_{ true };
    };
} // namespace cots::editor

#endif //CURSEOFTHESEA_EDITOR_STATS_PANEL_H
