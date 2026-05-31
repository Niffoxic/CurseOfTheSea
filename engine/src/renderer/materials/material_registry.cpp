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
#include "trishul/renderer/materials/material_registry.h"

#include <algorithm>
#include <cstring>

namespace trishul::render::materials
{
    shader_id material_registry::register_shader(const shader_desc& desc)
    {
        std::lock_guard lock(mutex_);

        //~ deduping on path plus entries the same shader always returns the same
        //~ id so the pso cache hit path stays stable
        for (std::uint32_t i = 0; i < shaders_.size(); ++i)
        {
            const auto& s = shaders_[i];
            if (!s.alive) continue;
            if (s.desc.path     == desc.path     &&
                s.desc.vs_entry == desc.vs_entry &&
                s.desc.ps_entry == desc.ps_entry)
            {
                return static_cast<shader_id>(i);
            }
        }

        shader_slot slot;
        slot.desc  = desc;
        slot.alive = true;
        shaders_.push_back(std::move(slot));
        return static_cast<shader_id>(shaders_.size() - 1u);
    }

    const shader_desc* material_registry::shader(const shader_id id) const
    {
        if (id < 0) return nullptr;
        std::lock_guard lock(mutex_);
        const auto idx = static_cast<std::uint32_t>(id);
        if (idx >= shaders_.size()) return nullptr;
        if (!shaders_[idx].alive)   return nullptr;
        return &shaders_[idx].desc;
    }

    std::uint32_t material_registry::shader_count() const noexcept
    {
        std::lock_guard lock(mutex_);
        return static_cast<std::uint32_t>(shaders_.size());
    }

    material_id material_registry::create(const shader_id shader)
    {
        std::lock_guard lock(mutex_);

        //~ reusing a freed slot when we can otherwise growing the deque which
        //~ leaves every existing slot put so old constants pointers stay valid
        std::uint32_t index;
        if (!mat_free_list_.empty())
        {
            index = mat_free_list_.back();
            mat_free_list_.pop_back();
        }
        else
        {
            index = static_cast<std::uint32_t>(mats_.size());
            mats_.emplace_back();
        }

        auto& s = mats_[index];
        s = {};                  //~ wiping the reused slot clean
        s.shader = shader;
        s.alive  = true;
        return static_cast<material_id>(index);
    }

    void material_registry::destroy(const material_id id)
    {
        if (id < 0) return;
        std::lock_guard lock(mutex_);
        const auto idx = static_cast<std::uint32_t>(id);
        if (idx >= mats_.size()) return;
        auto& s = mats_[idx];
        if (!s.alive) return;
        s.alive = false;
        mat_free_list_.push_back(idx);
    }

    void material_registry::set_shader(const material_id id, const shader_id shader)
    {
        if (id < 0) return;
        std::lock_guard lock(mutex_);
        const auto idx = static_cast<std::uint32_t>(id);
        if (idx >= mats_.size()) return;
        if (!mats_[idx].alive)   return;
        mats_[idx].shader = shader;
    }

    void material_registry::set_texture(const material_id id,
                                        const std::uint32_t slot,
                                        const std::uint32_t bindless_index)
    {
        if (id < 0) return;
        if (slot >= k_max_texture_slots) return;
        std::lock_guard lock(mutex_);
        const auto idx = static_cast<std::uint32_t>(id);
        if (idx >= mats_.size()) return;
        if (!mats_[idx].alive)   return;
        mats_[idx].gpu.slots[slot] = bindless_index;
    }

    void material_registry::set_params(const material_id id,
                                       const void* bytes,
                                       const std::size_t size)
    {
        if (id < 0 || !bytes || !size) return;
        std::lock_guard lock(mutex_);
        const auto idx = static_cast<std::uint32_t>(id);
        if (idx >= mats_.size()) return;
        if (!mats_[idx].alive)   return;
        //~ clamping to the fixed param size silently a shader only ever reads
        //~ what its cbuffer struct declares anyway
        const std::size_t to_copy = std::min<std::size_t>(size, k_max_param_bytes);
        std::memcpy(mats_[idx].gpu.params, bytes, to_copy);
    }

    shader_id material_registry::shader_of(const material_id id) const
    {
        if (id < 0) return invalid_shader;
        std::lock_guard lock(mutex_);
        const auto idx = static_cast<std::uint32_t>(id);
        if (idx >= mats_.size()) return invalid_shader;
        if (!mats_[idx].alive)   return invalid_shader;
        return mats_[idx].shader;
    }

    const gpu_material_cb* material_registry::constants(const material_id id) const
    {
        if (id < 0) return nullptr;
        std::lock_guard lock(mutex_);
        const auto idx = static_cast<std::uint32_t>(id);
        if (idx >= mats_.size()) return nullptr;
        if (!mats_[idx].alive)   return nullptr;
        return &mats_[idx].gpu;
    }

    bool material_registry::is_alive(const material_id id) const
    {
        if (id < 0) return false;
        std::lock_guard lock(mutex_);
        const auto idx = static_cast<std::uint32_t>(id);
        return idx < mats_.size() && mats_[idx].alive;
    }

    std::uint32_t material_registry::slot_count() const noexcept
    {
        std::lock_guard lock(mutex_);
        return static_cast<std::uint32_t>(mats_.size());
    }

    std::uint32_t material_registry::texture_at(const material_id id,
                                                const std::uint32_t slot) const
    {
        if (id < 0 || slot >= k_max_texture_slots) return 0u;
        std::lock_guard lock(mutex_);
        const auto idx = static_cast<std::uint32_t>(id);
        if (idx >= mats_.size() || !mats_[idx].alive) return 0u;
        return mats_[idx].gpu.slots[slot];
    }

    material_id material_registry::default_material() const noexcept
    {
        return default_id_;
    }

    void material_registry::set_default_material(const material_id id) noexcept
    {
        default_id_ = id;
    }
} // namespace trishul::render::materials
