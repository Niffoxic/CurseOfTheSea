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
// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_DEFERRED_RELEASER_H
#define CURSEOFTHESEA_DEFERRED_RELEASER_H

#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>
#include <wrl/client.h>

#include "trishul/core/interface/hardware.h"

struct IUnknown;
namespace D3D12MA { class Allocation; }

namespace trishul::render::hardware
{
    class descriptor_heap;

    struct deferred_releaser_config
    {
        descriptor_heap* bindless{ nullptr };
    };

    // deferred release queue for gpu resources
    // every runtime destruction enqueues an object stamped with the fence
    // value the currently building frame will signal the render thread
    // drains entries whose stamp is reached on the gpu side so nothing
    // freed inline is touched by a still in flight frame
    class deferred_releaser final: public interfaces
    {
    public:
        using fence_value = std::uint64_t;

         deferred_releaser() = default;
        ~deferred_releaser() override;

        deferred_releaser           (const deferred_releaser&) = delete;
        deferred_releaser& operator=(const deferred_releaser&) = delete;

        //~ lifecycle
        [[nodiscard]]
        bool initialize  ()          override;
        void deinitialize() noexcept override;

        [[nodiscard]] bool        need_rebuild() const noexcept override
        {
            return need_rebuild_.load(std::memory_order_acquire);
        }
        [[nodiscard]] const char* name() const noexcept override { return "deferred_releaser"; }

        void mark_for_rebuild() noexcept
        {
            need_rebuild_.store(true, std::memory_order_release);
        }

        //~ publish the fence value the currently building frame will signal
        // every enqueue after this point stamps with this value the render
        // thread updates this once per frame before recording starts
        void set_pending_fence(fence_value v) noexcept;
        [[nodiscard]] fence_value pending_fence() const noexcept;

        //~ queue takes ownership
        template<class T>
        void enqueue_com(Microsoft::WRL::ComPtr<T>&& obj)
        {
            if (!obj) return;
            Microsoft::WRL::ComPtr<IUnknown> upcast;
            upcast.Attach(obj.Detach());
            enqueue_com_impl(std::move(upcast));
        }

        //~ enqueue for a deferred Release
        void enqueue_allocation(D3D12MA::Allocation* a);

        //~ enqueue a bindless descriptor slot for deferred return
        void enqueue_bindless_slot(std::uint32_t slot);

        //~ render thread drain after wait releases every entry whose stamp
        // is reached on the gpu side returns the count released
        std::size_t drain(fence_value completed_value);

        //~ release every queued entry regardless of fence
        std::size_t drain_all();

        [[nodiscard]] std::size_t pending_count() const;

    private:
        void enqueue_com_impl(Microsoft::WRL::ComPtr<IUnknown>&& obj);

        enum class entry_kind : std::uint8_t
        {
            com,
            allocation,
            bindless_slot,
        };

        struct entry
        {
            entry_kind  k             { entry_kind::com };
            fence_value stamp         { 0 };
            std::uint32_t bindless_slot{ 0 };
            D3D12MA::Allocation*             allocation { nullptr };
            Microsoft::WRL::ComPtr<IUnknown> com_obj;
        };

        void release_entry(entry& e) const;

        mutable std::mutex       mutex_;
        std::vector<entry>       entries_;
        std::atomic<fence_value> pending_fence_ { 1 };
        descriptor_heap*         bindless_      { nullptr };
        std::atomic<bool>        need_rebuild_  { true };
    };
} // namespace trishul::render::hardware

#endif //CURSEOFTHESEA_DEFERRED_RELEASER_H
