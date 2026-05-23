// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_EDITOR_PANEL_H
#define CURSEOFTHESEA_EDITOR_PANEL_H

namespace cots::editor
{
    //~ retained engine side panel
    class panel
    {
    public:
        virtual ~panel() = default;

        panel() = default;
        panel(const panel&) = delete;
        panel(panel&&)      = delete;
        panel& operator=(const panel&) = delete;
        panel& operator=(panel&&)      = delete;

        //~ draw the panel
        virtual void draw() = 0;

        //~ short debug name
        [[nodiscard]]
        virtual const char* name() const noexcept = 0;
    };
} // namespace cots::editor

#endif //CURSEOFTHESEA_EDITOR_PANEL_H
