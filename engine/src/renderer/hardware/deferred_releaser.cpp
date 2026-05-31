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
#include "trishul/renderer/hardware/deferred_releaser.h"
#include "trishul/renderer/hardware/descriptor_heap.h"
#include "trishul/utils/logger.h"
#include "trishul/core/engine_assert.h"

#include <D3D12MemAlloc.h>
#include <algorithm>
#include <ranges>
#include <iterator>

namespace trishul::render::hardware
{
    deferred_releaser::~deferred_releaser()
    {
        deinitialize();
    }

    bool deferred_releaser::initialize()
    {
        //~ read the config the bindless heap we return freed slots to
        const auto* cfg = config_as<deferred_releaser_config>();
        ENGINE_ASSERT_MSG(cfg && cfg->bindless,
            "deferred_releaser config missing call set_config<deferred_releaser_config> first");
        bindless_ = cfg->bindless;

        pending_fence_.store(1u, std::memory_order_release);
        need_rebuild_ .store(false, std::memory_order_release);
        return true;
    }

    void deferred_releaser::deinitialize() noexcept
    {
        //~ flush any remaining work assumes the gpu is idle by now its a must
        //~ in the coordinated teardown we run before the heap and allocator die
        if (const std::size_t leaked = drain_all(); leaked > 0u)
        {
            LOG_INFO("drained {} entries on shutdown", leaked);
        }
        bindless_ = nullptr;
        need_rebuild_.store(true, std::memory_order_release); //~ reusable
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
        //~ split into still pending and ready to release
        std::vector<entry> to_release;
        {
            std::lock_guard lock(mutex_);
            if (entries_.empty()) return 0u;

            //~ stable_partition keeps still pending entries up front and returns
            //~ the subrange of the ready tail stamp reached on the gpu side note
            //~ ranges stable_partition returns a subrange
            auto ready =
                std::ranges::stable_partition(entries_,
                 [completed_value](const entry& e)
                 {
                     return e.stamp > completed_value;
                 });

            to_release.reserve(ready.size());
            std::move(ready.begin(), ready.end(), std::back_inserter(to_release));
            entries_.erase(ready.begin(), ready.end());
        }

        //~ release outside the lock allocation release can take a while tho!
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
} // namespace trishul::render::hardware
