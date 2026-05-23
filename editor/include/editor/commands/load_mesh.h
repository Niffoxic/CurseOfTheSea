// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_EDITOR_LOAD_MESH_H
#define CURSEOFTHESEA_EDITOR_LOAD_MESH_H

#include <string>
#include "editor/command.h"

namespace cots::editor
{
    //~ runtime mesh load
    class load_mesh final : public editor_command
    {
    public:
        load_mesh(std::string path, std::string label) noexcept;

        void execute() override;
        void undo()    override;

        [[nodiscard]] const char* name() const noexcept override
        {
            return "load_mesh";
        }

    private:
        std::string   path_;
        std::string   label_;
        std::uint32_t registered_id_{ 0xffffffffu };
    };
} // namespace cots::editor

#endif //CURSEOFTHESEA_EDITOR_LOAD_MESH_H
