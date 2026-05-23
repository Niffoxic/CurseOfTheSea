// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_EDITOR_LOAD_TEXTURE_H
#define CURSEOFTHESEA_EDITOR_LOAD_TEXTURE_H

#include <string>
#include "editor/command.h"

namespace cots::editor
{
    //~ runtime texture load
    class load_texture final : public editor_command
    {
    public:
        explicit load_texture(std::string path) noexcept;

        void execute() override;
        void undo()    override;

        [[nodiscard]] const char* name() const noexcept override
        {
            return "load_texture";
        }

    private:
        std::string path_;
        bool        loaded_{ false };
    };
} // namespace cots::editor

#endif //CURSEOFTHESEA_EDITOR_LOAD_TEXTURE_H
