// Created by Niffoxic (Harsh Dubey)
#include "editor/commands/delete_actor.h"

namespace cots::editor
{
    delete_actor::delete_actor(std::uint64_t id) noexcept
        : id_(id) {}

    void delete_actor::execute()
    {
        if (!had_saved_)
        {
            //~ capture the actor before removing
            for (const auto& a : world_instance().actors())
            {
                if (a.id == id_)
                {
                    saved_     = a;
                    had_saved_ = true;
                    break;
                }
            }
        }
        world_instance().remove(id_);
    }

    void delete_actor::undo()
    {
        if (!had_saved_) return;
        world_instance().restore(saved_);
    }
} // namespace cots::editor
