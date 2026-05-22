// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_DEPTH_TARGET_H
#define CURSEOFTHESEA_DEPTH_TARGET_H

#include <cstdint>
#include <wrl/client.h>

struct ID3D12Resource2;
struct ID3D12DescriptorHeap;

namespace D3D12MA { class Allocation; }

namespace cots::graphics::hardware
{
    class device;

    //~ depth-target resource: committed default-heap 2D texture with DSV
    //  format is D32_FLOAT_S8X24_UINT (reversed-Z + 8-bit stencil reserved)
    //  non-shader-visible DSV heap; no SRV until Phase 12 forces a depth read
    class depth_target final
    {
    public:
         depth_target() = default;
        ~depth_target();

        depth_target           (const depth_target&) = delete;
        depth_target& operator=(const depth_target&) = delete;

        [[nodiscard]] bool initialize  (const device& dev,
                                        std::uint32_t width,
                                        std::uint32_t height);
                      void deinitialize() noexcept;

        //~ release current resource + recreate at new size
        //  caller must ensure the GPU is idle first
        [[nodiscard]] bool resize(const device& dev,
                                  std::uint32_t width,
                                  std::uint32_t height);

        [[nodiscard]] ID3D12Resource2* resource  () const noexcept;
        [[nodiscard]] std::size_t      dsv_handle() const noexcept;

        [[nodiscard]] std::uint32_t    width () const noexcept { return width_;  }
        [[nodiscard]] std::uint32_t    height() const noexcept { return height_; }

    private:
        bool create_resource(const device& dev);
        void release_resource() noexcept;

    private:
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsv_heap_;
        Microsoft::WRL::ComPtr<ID3D12Resource2>      resource_;
        D3D12MA::Allocation*                         allocation_ { nullptr };

        std::uint32_t width_  { 0 };
        std::uint32_t height_ { 0 };
    };
} // namespace cots::graphics::hardware

#endif
