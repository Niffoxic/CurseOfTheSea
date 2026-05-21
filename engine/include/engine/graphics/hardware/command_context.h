// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_COMMAND_CONTEXT_H
#define CURSEOFTHESEA_COMMAND_CONTEXT_H

#include <cstdint>
#include <wrl/client.h>

#include "types.h"

struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList7;
struct ID3D12Resource2;

namespace cots::graphics::hardware
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

        [[nodiscard]] bool initialize  (const device& dev, command_list_type type = command_list_type::direct);
                      void deinitialize() noexcept;

        // reset allocator and list for a fresh frame
        // wait for gpu to finish work before working on it
        [[nodiscard]] bool reset();

        // close the list before executing
        // no more recording after this
        [[nodiscard]] bool close();

        // transition a resource between states TODO: Automatic transition
        void transition(ID3D12Resource2* resource,
                        resource_state from,
                        resource_state to) const;

        void clear_render_target(std::size_t rtv_handle, const float color[4]) const;
        void set_render_target  (std::size_t rtv_handle) const;

        [[nodiscard]] ID3D12GraphicsCommandList7* list() const noexcept;

    private:
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator>     allocator_;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> list_;
        command_list_type                                  type_    { command_list_type::direct };
        bool                                               is_open_ { false };
    };
} // namespace cots::graphics::hardware

#endif
