// Created by Niffoxic (Harsh Dubey)
#include "editor/world.h"
#include "engine/graphics/render_snapshot.h"

#include <algorithm>
#include <cstring>

namespace cots::editor
{
    std::uint64_t world::spawn(const actor& proto)
    {
        actor a    = proto;
        a.id       = next_id_++;
        a.alive    = true;
        actors_.push_back(a);
        return a.id;
    }

    bool world::remove(std::uint64_t id)
    {
        for (auto& a : actors_)
        {
            if (a.id == id && a.alive)
            {
                a.alive = false;
                return true;
            }
        }
        return false;
    }

    bool world::restore(const actor& a)
    {
        for (auto& existing : actors_)
        {
            if (existing.id == a.id)
            {
                existing       = a;
                existing.alive = true;
                return true;
            }
        }
        //~ id was reaped append a fresh slot
        actor copy    = a;
        copy.alive    = true;
        actors_.push_back(copy);
        if (a.id >= next_id_) next_id_ = a.id + 1;
        return true;
    }

    bool world::update(const actor& a)
    {
        for (auto& existing : actors_)
        {
            if (existing.id == a.id)
            {
                const bool was_alive = existing.alive;
                existing       = a;
                existing.alive = was_alive;
                return true;
            }
        }
        return false;
    }

    void world::publish(graphics::scene_snapshot& snap) const
    {
        for (const auto& a : actors_)
        {
            if (!a.alive) continue;
            graphics::mesh_instance mi{};
            std::memcpy(&mi.transform, &a.transform, sizeof(a.transform));
            mi.mesh_index     = a.mesh_index;
            mi.material_index = a.material_index;
            snap.instances.push_back(mi);
        }
    }

    std::size_t world::alive_count() const noexcept
    {
        std::size_t n = 0;
        for (const auto& a : actors_) if (a.alive) ++n;
        return n;
    }
} // namespace cots::editor
