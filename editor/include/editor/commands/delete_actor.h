// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_EDITOR_DELETE_ACTOR_H
#define CURSEOFTHESEA_EDITOR_DELETE_ACTOR_H

#include "editor/command.h"
#include "editor/world.h"

namespace cots::editor
{
    //~ remove an actor from the editor world
    class delete_actor final : public editor_command
    {
    public:
        explicit delete_actor(std::uint64_t id) noexcept;

        void execute() override;
        void undo()    override;

        [[nodiscard]] const char* name() const noexcept override
        {
            return "delete_actor";
        }

    private:
        std::uint64_t id_;
        actor         saved_{};
        bool          had_saved_{ false };
    };
} // namespace cots::editor

#endif //CURSEOFTHESEA_EDITOR_DELETE_ACTOR_H
