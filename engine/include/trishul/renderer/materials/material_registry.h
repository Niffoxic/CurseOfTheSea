//=============================================================================
// Curse of the Sea
//=============================================================================
// Created by  Niffoxic - Harsh Dubey
// Module      WM9M6 Fundamentals of Games Research Development and Management
// Institution University of Warwick
//
// A linear story driven pirate adventure built from scratch in C++23 and
// DirectX 12 for the University of Warwick game project assessment.
//=============================================================================
#ifndef CURSEOFTHESEA_MATERIAL_REGISTRY_H
#define CURSEOFTHESEA_MATERIAL_REGISTRY_H

#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

#include "trishul/renderer/materials/material_layout.h"

namespace trishul::render::materials
{
    using shader_id = std::int32_t;
    constexpr shader_id invalid_shader = -1;

    using material_id = std::int32_t;
    constexpr material_id invalid_material = -1;

    //~ a registered shader just points at hlsl on disk the pso cache resolves
    //~ vs and ps from these entry points when it first needs the pso
    struct shader_desc
    {
        std::string path;                //~ hlsl source path
        std::string vs_entry { "VSMain" };
        std::string ps_entry { "PSMain" };
    };

    //~ the engine side material registry slot based and the main thing the world
    //~ editor pokes at a material is just a shader plus eight bindless texture
    //~ slots plus 128 bytes of opaque params the shader reads the slots and
    //~ params however it likes the registry never interprets them
    //~
    //~ thread safe on purpose the game c abi can drive it from the main thread
    //~ while the render thread reads it in the mesh pass
    class material_registry final
    {
    public:
        material_registry() = default;
        ~material_registry() = default;

        material_registry           (const material_registry&) = delete;
        material_registry& operator=(const material_registry&) = delete;

        //~ registering a shader handing back a stable id the same path and
        //~ entries always return the same id so the pso cache hit path is stable
        //~ nothing compiles here that waits for the first pso
        shader_id register_shader(const shader_desc& desc);
        [[nodiscard]] const shader_desc* shader(shader_id id) const;
        [[nodiscard]] std::uint32_t      shader_count() const noexcept;

        //~ material lifecycle the editor creates and destroys these freely
        material_id create(shader_id shader);
        void        destroy(material_id id);

        //~ mutation the editor calls these live while the game runs
        void set_shader (material_id id, shader_id shader);
        void set_texture(material_id id, std::uint32_t slot, std::uint32_t bindless_index);
        void set_params (material_id id, const void* bytes, std::size_t size);

        //~ queries the constants pointer stays valid until that id is destroyed
        //~ mats live in a deque so growing the table never moves existing slots
        //~ the lock is only held for the duration of the call
        [[nodiscard]] shader_id              shader_of(material_id id) const;
        [[nodiscard]] const gpu_material_cb* constants(material_id id) const;

        //~ editor friendly read backs so a panel can list and inspect materials
        //~ without poking the internals is_alive tells a stale handle from a
        //~ live one slot_count bounds an iteration over every slot ever made
        [[nodiscard]] bool          is_alive(material_id id) const;
        [[nodiscard]] std::uint32_t slot_count() const noexcept;
        [[nodiscard]] std::uint32_t texture_at(material_id id, std::uint32_t slot) const;

        //~ the fallback material a draw lands on when it names an invalid id
        [[nodiscard]] material_id default_material() const noexcept;
        void                      set_default_material(material_id id) noexcept;

    private:
        struct shader_slot
        {
            shader_desc desc {};
            bool        alive { false };
        };

        struct mat_slot
        {
            shader_id        shader { invalid_shader };
            gpu_material_cb  gpu    {};
            bool             alive  { false };
        };

        //~ both tables are deques so the pointers we hand out of shader() and
        //~ constants() stay put deques never relocate their existing elements
        //~ on a push_back the way a vector does
        mutable std::mutex          mutex_;
        std::deque<shader_slot>     shaders_;
        std::deque<mat_slot>        mats_;
        std::vector<std::uint32_t>  mat_free_list_;
        material_id                 default_id_ { invalid_material };
    };
} // namespace trishul::render::materials

#endif //CURSEOFTHESEA_MATERIAL_REGISTRY_H
