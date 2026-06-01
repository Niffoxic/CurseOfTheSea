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
#ifndef CURSEOFTHESEA_HDR_TARGET_H
#define CURSEOFTHESEA_HDR_TARGET_H

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <wrl/client.h>
#include <dxgiformat.h>

#include "trishul/core/interface/hardware.h"

struct ID3D12Resource2;
struct ID3D12DescriptorHeap;

namespace D3D12MA { class Allocation; }

namespace trishul::render::hardware
{
    class device;
    class descriptor_heap;
} // namespace trishul::render::hardware

namespace trishul::render::resource
{
    //~ initialization config
    struct hdr_target_config
    {
        const hardware::device*    dev      { nullptr };
        hardware::descriptor_heap* bindless { nullptr };
        std::uint32_t              width    { 1u };
        std::uint32_t              height   { 1u };
    };

    //~ committed default heap float color target the forward passes write into
    //  this in HDR space
    class hdr_target final : public hardware::interfaces
    {
    public:
        static constexpr DXGI_FORMAT k_format = DXGI_FORMAT_R16G16B16A16_FLOAT;

         hdr_target() = default;
        ~hdr_target() override;

        hdr_target           (const hdr_target&) = delete;
        hdr_target& operator=(const hdr_target&) = delete;
        hdr_target           (hdr_target&&)      = delete;
        hdr_target& operator=(hdr_target&&)      = delete;

        //~ lifecycle
        [[nodiscard]]
        bool initialize  ()          override;
        void deinitialize() noexcept override;

        [[nodiscard]] bool need_rebuild() const noexcept override
        {
            return need_rebuild_.load(std::memory_order_acquire);
        }
        [[nodiscard]] const char* name() const noexcept override { return "hdr_target"; }

        void mark_for_rebuild() noexcept
        {
            need_rebuild_.store(true, std::memory_order_release);
        }

        //~ release and recreate at new size keeping the bindless slot gpu must be
        //~ idle before calling this
        [[nodiscard]] bool resize(std::uint32_t width, std::uint32_t height);

        [[nodiscard]] ID3D12Resource2* resource    () const noexcept;
        [[nodiscard]] std::size_t      rtv_handle  () const noexcept;
        [[nodiscard]] std::uint32_t    bindless_srv() const noexcept { return srv_slot_; }

        [[nodiscard]] std::uint32_t    width () const noexcept { return width_;  }
        [[nodiscard]] std::uint32_t    height() const noexcept { return height_; }

    private:
        bool create_resource ();
        void release_resource() noexcept;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_heap_;
        Microsoft::WRL::ComPtr<ID3D12Resource2>      resource_;
        D3D12MA::Allocation*                         allocation_ { nullptr };

        const hardware::device*                      device_   { nullptr };
        hardware::descriptor_heap*                   bindless_ { nullptr };
        std::uint32_t                                srv_slot_ { ~0u };

        std::uint32_t width_  { 1u };
        std::uint32_t height_ { 1u };

        std::atomic<bool> need_rebuild_{ true };
    };
} // namespace trishul::render::resource

#endif //CURSEOFTHESEA_HDR_TARGET_H
