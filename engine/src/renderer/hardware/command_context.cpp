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
#include "trishul/renderer/hardware/command_context.h"
#include "trishul/renderer/hardware/device.h"
#include "trishul/core/exception/dx_exception.h"
#include "trishul/utils/statics.h"
#include "trishul/utils/logger.h"

#include <d3d12.h>

namespace trishul::render::hardware
{
    command_context::~command_context()
    {
        deinitialize();
    }

    bool command_context::initialize()
    {
        if (list_) return true;

        //~ config
        const auto* cfg = config_as<command_context_config>();
        if (!cfg || !cfg->dev)
        {
            LOG_ERROR("command_context missing config or device");
            return false;
        }

        auto* d3d = cfg->dev->d3d12_device();
        if (!d3d)
        {
            LOG_ERROR("device not initialized");
            return false;
        }

        type_ = cfg->type;
        const D3D12_COMMAND_LIST_TYPE d3d_type = statics::to_d3d12(type_);

        try
        {
            DX_THROW_IF_FAILED_MSG(
                d3d->CreateCommandAllocator(d3d_type, IID_PPV_ARGS(&allocator_)),
                "failed to create command allocator for context");

            DX_THROW_IF_FAILED_MSG(
                d3d->CreateCommandList(0, d3d_type, allocator_.Get(),
                                       nullptr, IID_PPV_ARGS(&list_)),
                "failed to create command list for contexts");

            //~ closing for a clean reset later
            DX_THROW_IF_FAILED_MSG(list_->Close(), "initial Close");
            is_open_ = false;

            (void)allocator_->SetName(L"COTS Command Allocator");
            (void)list_     ->SetName(L"COTS Command List");

            need_rebuild_.store(false, std::memory_order_release);
            return true;
        }
        catch (const exception::directx& e)
        {
            LOG_ERROR("init failed: {}", e.what());
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
        //~ no device backing yet nothing to reset bail before the null deref
        if (!allocator_ || !list_)
        {
            LOG_WARN("reset on an uninitialized command_context");
            return false;
        }

        if (is_open_)
        {
            LOG_WARN("reset called while list still open");
            return false;
        }

        if (const HRESULT alloc_hr = allocator_->Reset(); FAILED(alloc_hr))
        {
            LOG_ERROR("allocator Reset failed {:08X}",
                          static_cast<std::uint32_t>(alloc_hr));
            return false;
        }

        if (const HRESULT list_hr = list_->Reset(allocator_.Get(), nullptr); FAILED(list_hr))
        {
            LOG_ERROR("list Reset failed {:08X}",
                          static_cast<std::uint32_t>(list_hr));
            return false;
        }

        is_open_ = true;
        return true;
    }

    bool command_context::close()
    {
        if (!list_)    return false;
        if (!is_open_) return true;

        if (const HRESULT hr = list_->Close(); FAILED(hr))
        {
            LOG_ERROR("Close failed {:08X}",
                          static_cast<std::uint32_t>(hr));
            return false;
        }
        is_open_ = false;
        return true;
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

    void command_context::set_descriptor_heap(ID3D12DescriptorHeap* heap) const
    {
        if (!is_open_ || !heap) return;
        list_->SetDescriptorHeaps(1, &heap);
    }

    ID3D12GraphicsCommandList7* command_context::list() const noexcept
    {
        return list_.Get();
    }
} // namespace trishul::render::hardware
