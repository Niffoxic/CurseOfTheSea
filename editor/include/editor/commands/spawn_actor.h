// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_EDITOR_SPAWN_ACTOR_H
#define CURSEOFTHESEA_EDITOR_SPAWN_ACTOR_H

#include "editor/command.h"
#include "editor/world.h"

namespace cots::editor
{
    //~ create an actor in the editor world
    class spawn_actor final : public editor_command
    {
    public:
        explicit spawn_actor(actor proto) noexcept;

        void execute() override;
        void undo()    override;

        [[nodiscard]] const char* name() const noexcept override
        {
            return "spawn_actor";
        }

        [[nodiscard]] std::uint64_t spawned_id() const noexcept { return spawned_id_; }

    private:
        actor         proto_;
        std::uint64_t spawned_id_{ 0 };
    };
} // namespace cots::editor

#endif //CURSEOFTHESEA_EDITOR_SPAWN_ACTOR_H
