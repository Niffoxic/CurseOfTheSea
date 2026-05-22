// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/hardware/command_context.h"
#include "engine/graphics/hardware/device.h"
#include "engine/graphics/utils/exception.h"
#include "engine/utils/helpers.h"

#include <d3d12.h>
#include <spdlog/spdlog.h>

namespace cots::graphics::hardware
{
    command_context::~command_context()
    {
        deinitialize();
    }

    bool command_context::initialize(const device& dev, const command_list_type type)
    {
        if (list_) return true;

        auto* d3d = dev.d3d12_device();
        if (!d3d)
        {
            spdlog::error("[hardware:cmd] device not initialized");
            return false;
        }

        type_ = type;
        const D3D12_COMMAND_LIST_TYPE d3d_type = helpers::to_d3d12(type);

        try
        {
            COTS_DX_THROW_IF_FAILED_MSG(
                d3d->CreateCommandAllocator(d3d_type, IID_PPV_ARGS(&allocator_)),
                "CreateCommandAllocator");

            COTS_DX_THROW_IF_FAILED_MSG(
                d3d->CreateCommandList(0, d3d_type, allocator_.Get(),
                                       nullptr, IID_PPV_ARGS(&list_)),
                "CreateCommandList");

            //~ closing for a clean reset later
            COTS_DX_THROW_IF_FAILED_MSG(list_->Close(), "initial Close");
            is_open_ = false;

            allocator_->SetName(L"COTS Command Allocator");
            list_     ->SetName(L"COTS Command List");
            return true;
        }
        catch (const exception& e)
        {
            spdlog::error("[hardware:cmd] init failed: {}", e.what());
            deinitialize();
            return false;
        }
    }

    void command_context::deinitialize() noexcept
    {
        list_.Reset();
        allocator_.Reset();
        is_open_ = false;
    }

    bool command_context::reset()
    {
        if (is_open_)
        {
            spdlog::warn("[hardware:cmd] reset called while list still open");
            return false;
        }

        if (const HRESULT alloc_hr = allocator_->Reset(); FAILED(alloc_hr))
        {
            spdlog::error("[hardware:cmd] allocator Reset failed (hr=0x{:08X})",
                          static_cast<std::uint32_t>(alloc_hr));
            return false;
        }

        if (const HRESULT list_hr = list_->Reset(allocator_.Get(), nullptr); FAILED(list_hr))
        {
            spdlog::error("[hardware:cmd] list Reset failed (hr=0x{:08X})",
                          static_cast<std::uint32_t>(list_hr));
            return false;
        }

        is_open_ = true;
        return true;
    }

    bool command_context::close()
    {
        if (!is_open_) return true;

        if (const HRESULT hr = list_->Close(); FAILED(hr))
        {
            spdlog::error("[hardware:cmd] Close failed (hr=0x{:08X})",
                          static_cast<std::uint32_t>(hr));
            return false;
        }
        is_open_ = false;
        return true;
    }

    void command_context::transition(ID3D12Resource2* resource,
                                     const resource_state from,
                                     const resource_state to) const
    {
        if (!resource || !is_open_) return;

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource   = resource;
        barrier.Transition.StateBefore = helpers::to_d3d12(from);
        barrier.Transition.StateAfter  = helpers::to_d3d12(to);
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        list_->ResourceBarrier(1, &barrier);
    }

    void command_context::clear_render_target(
        const std::size_t rtv_handle,
        const float color[4]
    ) const
    {
        if (!is_open_) return;

        const D3D12_CPU_DESCRIPTOR_HANDLE handle{ rtv_handle };
        list_->ClearRenderTargetView(handle, color, 0, nullptr);
    }

    void command_context::clear_depth_stencil(
        const std::size_t   dsv_handle,
        const float         depth,
        const std::uint8_t  stencil
    ) const
    {
        if (!is_open_ || dsv_handle == 0) return;

        const D3D12_CPU_DESCRIPTOR_HANDLE handle{ dsv_handle };
        constexpr D3D12_CLEAR_FLAGS flags =
            D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL;
        list_->ClearDepthStencilView(handle, flags, depth, stencil, 0, nullptr);
    }

    void command_context::set_render_target(const std::size_t rtv_handle) const
    {
        if (!is_open_) return;

        const D3D12_CPU_DESCRIPTOR_HANDLE handle{ rtv_handle };

        list_->OMSetRenderTargets(1,
            &handle,
            FALSE,
            nullptr
        );
    }

    void command_context::set_render_target(
        const std::size_t rtv_handle,
        const std::size_t dsv_handle
    ) const
    {
        if (!is_open_) return;

        const D3D12_CPU_DESCRIPTOR_HANDLE rtv{ rtv_handle };
        const D3D12_CPU_DESCRIPTOR_HANDLE dsv{ dsv_handle };

        list_->OMSetRenderTargets(1,
            &rtv,
            FALSE,
            dsv_handle ? &dsv : nullptr
        );
    }

    ID3D12GraphicsCommandList7* command_context::list() const noexcept
    {
        return list_.Get();
    }
} // namespace cots::graphics::hardware
