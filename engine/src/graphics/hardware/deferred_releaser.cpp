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
#include "engine/graphics/hardware/deferred_releaser.h"
#include "engine/graphics/hardware/descriptor_heap.h"

#include <D3D12MemAlloc.h>
#include <algorithm>
#include <spdlog/spdlog.h>

namespace cots::graphics::hardware
{
    deferred_releaser::~deferred_releaser()
    {
        deinitialize();
    }

    void deferred_releaser::initialize(descriptor_heap& bindless)
    {
        bindless_ = &bindless;
        pending_fence_.store(1u, std::memory_order_release);
    }

    void deferred_releaser::deinitialize()
    {
        const std::size_t leaked = drain_all();
        if (leaked > 0u)
        {
            spdlog::info("[deferred] drained {} entries on shutdown", leaked);
        }
        bindless_ = nullptr;
    }

    void deferred_releaser::set_pending_fence(const fence_value v) noexcept
    {
        pending_fence_.store(v, std::memory_order_release);
    }

    deferred_releaser::fence_value
        deferred_releaser::pending_fence() const noexcept
    {
        return pending_fence_.load(std::memory_order_acquire);
    }

    void deferred_releaser::enqueue_com_impl(Microsoft::WRL::ComPtr<IUnknown>&& obj)
    {
        std::lock_guard lock(mutex_);
        entry e{};
        e.k       = entry_kind::com;
        e.stamp   = pending_fence_.load(std::memory_order_acquire);
        e.com_obj = std::move(obj);
        entries_.push_back(std::move(e));
    }

    void deferred_releaser::enqueue_allocation(D3D12MA::Allocation* a)
    {
        if (!a) return;
        std::lock_guard lock(mutex_);
        entry e{};
        e.k          = entry_kind::allocation;
        e.stamp      = pending_fence_.load(std::memory_order_acquire);
        e.allocation = a;
        entries_.push_back(std::move(e));
    }

    void deferred_releaser::enqueue_bindless_slot(const std::uint32_t slot)
    {
        if (slot == descriptor_heap::invalid_slot) return;
        std::lock_guard lock(mutex_);
        entry e{};
        e.k             = entry_kind::bindless_slot;
        e.stamp         = pending_fence_.load(std::memory_order_acquire);
        e.bindless_slot = slot;
        entries_.push_back(std::move(e));
    }

    void deferred_releaser::release_entry(entry& e) const
    {
        switch (e.k)
        {
        case entry_kind::com:
            e.com_obj.Reset();
            break;
        case entry_kind::allocation:
            if (e.allocation)
            {
                e.allocation->Release();
                e.allocation = nullptr;
            }
            break;
        case entry_kind::bindless_slot:
            if (bindless_)
            {
                bindless_->release(e.bindless_slot);
            }
            break;
        }
    }

    std::size_t deferred_releaser::drain(const fence_value completed_value)
    {
        std::vector<entry> to_release;
        {
            std::lock_guard lock(mutex_);
            if (entries_.empty()) return 0u;

            const auto split = std::stable_partition(
                entries_.begin(), entries_.end(),
                [completed_value](const entry& e)
                {
                    return e.stamp > completed_value;
                });

            to_release.reserve(static_cast<std::size_t>(entries_.end() - split));
            std::move(split, entries_.end(), std::back_inserter(to_release));
            entries_.erase(split, entries_.end());
        }

        for (auto& e : to_release) release_entry(e);
        return to_release.size();
    }

    std::size_t deferred_releaser::drain_all()
    {
        std::vector<entry> to_release;
        {
            std::lock_guard lock(mutex_);
            to_release = std::move(entries_);
            entries_.clear();
        }
        for (auto& e : to_release) release_entry(e);
        return to_release.size();
    }

    std::size_t deferred_releaser::pending_count() const
    {
        std::lock_guard lock(mutex_);
        return entries_.size();
    }
} // namespace cots::graphics::hardware
