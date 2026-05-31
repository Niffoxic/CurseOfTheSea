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
#ifndef CURSEOFTHESEA_DEPTH_TARGET_H
#define CURSEOFTHESEA_DEPTH_TARGET_H

#include <atomic>
#include <cstdint>
#include <dxgiformat.h>
#include <wrl/client.h>

#include "trishul/core/interface/hardware.h"

struct ID3D12Resource2;
struct ID3D12DescriptorHeap;

namespace D3D12MA { class Allocation; }

namespace trishul::render::hardware
{
    class device;
    class descriptor_heap;

    struct depth_target_config
    {
        const device*    dev      { nullptr };
        descriptor_heap* bindless { nullptr };
        std::uint32_t    width    { 1u };
        std::uint32_t    height   { 1u };
    };

    //~ reversed Z depth stencil sampleable from shaders through a bindless srv
    //  the resource is typeless so that it can be used both as a dsv and srv
    class depth_target final: public interfaces
    {
    public:
        //~ typeless backing lets the same texture be a dsv and an srv
        static constexpr DXGI_FORMAT resource_format = DXGI_FORMAT_R32G8X24_TYPELESS;
        static constexpr DXGI_FORMAT dsv_format      = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        static constexpr DXGI_FORMAT srv_format      = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

        static constexpr std::uint32_t invalid_slot = ~0u;

         depth_target() = default;
        ~depth_target() override;

        depth_target           (const depth_target&) = delete;
        depth_target& operator=(const depth_target&) = delete;
        depth_target           (depth_target&&)      = delete;
        depth_target& operator=(depth_target&&)      = delete;

        //~ lifecycle
        [[nodiscard]]
        bool initialize  ()          override;
        void deinitialize() noexcept override;

        [[nodiscard]] bool        need_rebuild() const noexcept override
        {
            return need_rebuild_.load(std::memory_order_acquire);
        }
        [[nodiscard]] const char* name() const noexcept override { return "depth_target"; }

        //~ flag a rebuild eg the device came back on a new adapter
        void mark_for_rebuild() noexcept
        {
            need_rebuild_.store(true, std::memory_order_release);
        }

        //~ release current resource and recreate at the new size keeps the
        //  bindless slot caller must make sure the gpu is idle first
        [[nodiscard]] bool resize(std::uint32_t width, std::uint32_t height);

        [[nodiscard]] ID3D12Resource2* resource  () const noexcept;
        [[nodiscard]] std::size_t      dsv_handle() const noexcept; //~ cpu handle for the dsv

        //~ bindless slot for sampling invalid_slot when no bindless heap was given
        [[nodiscard]] std::uint32_t    srv_index () const noexcept { return srv_slot_; }

        [[nodiscard]] std::uint32_t    width () const noexcept { return width_;  }
        [[nodiscard]] std::uint32_t    height() const noexcept { return height_; }

    private:
        bool build();             //~ creates dsv heap, resource, dsv and srv
        bool create_dsv_heap();
        bool create_resource ();
        bool create_dsv      () const;
        bool create_srv      ();  //~ acquires a slot if needed then writes the view
        void release_resource() noexcept;
        void release_srv     () noexcept;

    private:
        const device*    device_   { nullptr };
        descriptor_heap* bindless_ { nullptr };

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsv_heap_;
        Microsoft::WRL::ComPtr<ID3D12Resource2>      resource_;
        D3D12MA::Allocation*                         allocation_ { nullptr };

        std::uint32_t width_    { 1u };
        std::uint32_t height_   { 1u };
        std::uint32_t srv_slot_ { invalid_slot };

        std::atomic<bool> need_rebuild_{ true };
    };
} // namespace trishul::render::hardware

#endif //CURSEOFTHESEA_DEPTH_TARGET_H