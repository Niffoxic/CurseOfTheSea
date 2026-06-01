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


#include <cstdint>
#include <wrl/client.h>
#include "types.h"

struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList7;
struct ID3D12DescriptorHeap;

namespace trishul::render::hardware
{
    class device;

    class command_context final
    {
    public:
         command_context() = default;
        ~command_context();

        command_context(const command_context&) = delete;
        command_context(command_context&&)      = delete;

        command_context& operator=(const command_context&) = delete;
        command_context& operator=(command_context&&)      = delete;

        [[nodiscard]]
        bool initialize  (const device& dev, command_list_type type = command_list_type::direct);
        void deinitialize() noexcept;

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

    private:
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator>     allocator_;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> list_;
        command_list_type                                  type_    { command_list_type::direct };
        bool                                               is_open_ { false };
    };
} // namespace trishul::render::hardware

#endif //CURSEOFTHESEA_COMMAND_CONTEXT_H
