// Created by Niffoxic (Harsh Dubey)
#include "editor/commands/spawn_actor.h"

namespace cots::editor
{
    spawn_actor::spawn_actor(actor proto) noexcept
        : proto_(proto) {}

    void spawn_actor::execute()
    {
        if (spawned_id_ != 0)
        {
            //~ redo path use the same id
            actor a       = proto_;
            a.id          = spawned_id_;
            a.alive       = true;
            world_instance().restore(a);
            return;
        }
        spawned_id_ = world_instance().spawn(proto_);
    }

    void spawn_actor::undo()
    {
        if (spawned_id_ == 0) return;
        world_instance().remove(spawned_id_);
    }
} // namespace cots::editor
