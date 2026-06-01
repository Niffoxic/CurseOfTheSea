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
#ifndef CURSEOFTHESEA_COMMAND_CONTEXT_H
#define CURSEOFTHESEA_COMMAND_CONTEXT_H

#include <atomic>
#include <cstdint>
#include <wrl/client.h>

#include "types.h"
#include "trishul/core/interface/hardware.h"

struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList7;
struct ID3D12DescriptorHeap;

namespace trishul::render::hardware
{
    class device;

    //~ config need to initialize and rebuild
    struct command_context_config
    {
        const device*     dev  { nullptr };
        command_list_type type { command_list_type::direct };
    };

    //~ one allocator plus one list living on the hardware interface
    class command_context final : public interfaces
    {
    public:
         command_context() = default;
        ~command_context() override;

        command_context(const command_context&) = delete;
        command_context(command_context&&)      = delete;

        command_context& operator=(const command_context&) = delete;
        command_context& operator=(command_context&&)      = delete;

        //~ lifecycle
        [[nodiscard]]
        bool initialize  ()          override;
        void deinitialize() noexcept override;

        [[nodiscard]] bool need_rebuild() const noexcept override
        {
            return need_rebuild_.load(std::memory_order_acquire);
        }
        [[nodiscard]] const char* name() const noexcept override { return "command_context"; }

        //~ flags a rebuild
        void mark_for_rebuild() noexcept
        {
            need_rebuild_.store(true, std::memory_order_release);
        }

        //~ reset the allocator and the list for a fresh frame
        // wait for gpu to finish work before working on it
        [[nodiscard]] bool reset();

        // close the list before executing
        // no more recording after this
        [[nodiscard]] bool close();

        void clear_render_target(std::size_t rtv_handle, const float color[4])              const;
        void clear_depth_stencil(std::size_t dsv_handle, float depth, std::uint8_t stencil) const;
        void set_render_target  (std::size_t rtv_handle)                                    const;
        void set_render_target  (std::size_t rtv_handle, std::size_t dsv_handle)            const;

        //~ bind the bindless heap
        void set_descriptor_heap(ID3D12DescriptorHeap* heap) const;

        [[nodiscard]] ID3D12GraphicsCommandList7* list() const noexcept;

        //~ true once an allocator and list actually exist on the device
        [[nodiscard]] bool is_valid         () const noexcept { return list_ != nullptr; }
        //~ true while were recording between reset and close
        [[nodiscard]] bool is_recording     () const noexcept { return is_open_; }
        [[nodiscard]] command_list_type type() const noexcept { return type_; }

    private:
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator>     allocator_;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> list_;
        command_list_type                                  type_    { command_list_type::direct };
        bool                                               is_open_ { false };
        std::atomic<bool>                                  need_rebuild_{ true };
    };
} // namespace trishul::render::hardware

#endif //CURSEOFTHESEA_COMMAND_CONTEXT_H