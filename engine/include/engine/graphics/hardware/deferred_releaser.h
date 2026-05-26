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
#ifndef CURSEOFTHESEA_DEFERRED_RELEASER_H
#define CURSEOFTHESEA_DEFERRED_RELEASER_H

#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>
#include <wrl/client.h>

struct IUnknown;
namespace D3D12MA { class Allocation; }

namespace cots::graphics::hardware
{
    class descriptor_heap;

    class deferred_releaser final
    {
    public:
        using fence_value = std::uint64_t;

        deferred_releaser() = default;
        ~deferred_releaser();

        deferred_releaser           (const deferred_releaser&) = delete;
        deferred_releaser& operator=(const deferred_releaser&) = delete;

        void initialize  (descriptor_heap& bindless);
        void deinitialize();

        void set_pending_fence(fence_value v) noexcept;
        [[nodiscard]] fence_value pending_fence() const noexcept;

        template<class T>
        void enqueue_com(Microsoft::WRL::ComPtr<T>&& obj)
        {
            if (!obj) return;
            Microsoft::WRL::ComPtr<IUnknown> upcast;
            upcast.Attach(obj.Detach());
            enqueue_com_impl(std::move(upcast));
        }

        void enqueue_allocation   (D3D12MA::Allocation* a);
        void enqueue_bindless_slot(std::uint32_t slot);

        std::size_t drain(fence_value completed_value);

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
    };
} // namespace cots::graphics::hardware

#endif //CURSEOFTHESEA_DEFERRED_RELEASER_H
