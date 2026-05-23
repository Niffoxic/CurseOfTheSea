// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_EDITOR_WORLD_H
#define CURSEOFTHESEA_EDITOR_WORLD_H

#include <cstdint>
#include <vector>
#include <DirectXMath.h>

namespace cots::graphics { struct scene_snapshot; }

namespace cots::editor
{
    //~ minimal editor side actor
    struct actor
    {
        std::uint64_t        id            { 0 };
        DirectX::XMFLOAT4X4  transform     {};
        std::uint32_t        mesh_index    { 0 };
        std::uint32_t        material_index{ 0 };
        bool                 alive         { true };
    };

    //~ editor side actor table
    class world
    {
    public:
        //~ allocate an actor and return its id
        std::uint64_t spawn(const actor& proto);

        //~ flag an actor dead
        bool remove(std::uint64_t id);

        //~ re insert a removed actor preserves its id
        bool restore(const actor& a);

        //~ overwrite an existing actor by id
        bool update(const actor& a);

        //~ append every alive actor into the snapshot instances vector
        void publish(graphics::scene_snapshot& snap) const;

        [[nodiscard]] const std::vector<actor>&  actors() const noexcept
        {
            return actors_;
        }
        [[nodiscard]] std::size_t alive_count() const noexcept;

    private:
        std::vector<actor> actors_;
        std::uint64_t      next_id_{ 1 };
    };

    //~ singleton accessor lives in the editor
    world& world_instance();
} // namespace cots::editor

#endif //CURSEOFTHESEA_EDITOR_WORLD_H
