// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_SWAPCHAIN_H
#define CURSEOFTHESEA_SWAPCHAIN_H

#include <array>
#include <cstdint>
#include <wrl/client.h>
#include <windows.h>

#include "types.h"
#include "device.h"

struct IDXGISwapChain4;
struct IDXGIOutput6;
struct ID3D12Resource2;
struct ID3D12DescriptorHeap;

namespace cots::graphics::hardware
{
    struct swapchain_create_info
    {
        HWND           window_handle { nullptr };
        std::uint32_t  width         { 1280 };
        std::uint32_t  height        { 720  };
        display_mode   mode          { display_mode::windowed };
        std::uint32_t  frame_count   { 3 };
        bool           allow_tearing { true };
        bool           hdr           { false };
        std::uint32_t  output_index  { 0 };  //~ for exclusive fullscreen
        display_format exclusive_mode{};
    };

    class swapchain final
    {
    public:
         swapchain() = default;
        ~swapchain();

        swapchain(const swapchain&) = delete;
        swapchain(swapchain&&)      = delete;

        swapchain& operator=(const swapchain&) = delete;
        swapchain& operator=(swapchain&&)      = delete;

        [[nodiscard]] bool initialize  (const device& dev, const swapchain_create_info& info);
                      void deinitialize() noexcept;

        //~ fast path - just resize backbuffers, keep mode & format
        [[nodiscard]] bool resize(const device& dev, std::uint32_t width, std::uint32_t height);

        //~ full recreate - new params, full teardown
        [[nodiscard]] bool recreate(const device& dev, const swapchain_create_info& info);

        //~ mode transitions
        [[nodiscard]] bool set_display_mode      (const device& dev, display_mode mode);
        [[nodiscard]] bool set_exclusive_mode    (const device& dev, std::uint32_t output_index,
                                                  const display_format& format);
        [[nodiscard]] bool set_windowed_size     (const device& dev, std::uint32_t width, std::uint32_t height);

        //~ per-frame
        bool present(std::uint32_t sync_interval = 0);

        //~ occlusion - call once per frame at start.
        //  returns true if rendering should proceed, false if occluded
        [[nodiscard]] bool check_occlusion();

        //~ accessors
        [[nodiscard]] std::uint32_t      current_backbuffer_index() const;
        [[nodiscard]] ID3D12Resource2*   current_backbuffer      () const;
        [[nodiscard]] std::size_t        current_rtv_handle      () const;

        [[nodiscard]] std::uint32_t      width () const noexcept { return width_; }
        [[nodiscard]] std::uint32_t      height() const noexcept { return height_; }
        [[nodiscard]] display_mode       current_mode  () const noexcept { return current_mode_; }
        [[nodiscard]] const display_format& current_format() const noexcept { return current_format_; }
        [[nodiscard]] std::uint32_t      current_output_index() const noexcept { return current_output_index_; }
        [[nodiscard]] bool               is_occluded() const noexcept { return is_occluded_; }
        [[nodiscard]] bool               tearing_supported() const noexcept { return tearing_supported_; }

        [[nodiscard]] IDXGISwapChain4*   dxgi_swapchain() const noexcept;

    private:
        bool create_swapchain   (const device& dev, const swapchain_create_info& info);
        bool create_backbuffer_views(const device& dev);
        void release_backbuffers() noexcept;

        bool apply_borderless   (const device& dev, const swapchain_create_info& info);
        bool apply_exclusive    (const device& dev, const swapchain_create_info& info);
        bool apply_windowed     (const device& dev, const swapchain_create_info& info);

        bool find_output        (const device& dev, std::uint32_t index,
                                 Microsoft::WRL::ComPtr<IDXGIOutput6>& out) const;

        static display_format pick_closest_mode(const output_info& out,
                                                const display_format& requested);

    private:
        Microsoft::WRL::ComPtr<IDXGISwapChain4>      swapchain_;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_heap_;
        std::array<Microsoft::WRL::ComPtr<ID3D12Resource2>, 8> backbuffers_;  //~ up to 8 buffers xD

        HWND          window_handle_       { nullptr };
        std::uint32_t width_               { 1 };
        std::uint32_t height_              { 1 };
        std::uint32_t frame_count_         { 3 };
        std::uint32_t rtv_descriptor_size_ { 0 };
        std::uint32_t current_output_index_{ 0 };

        display_mode   current_mode_       { display_mode::windowed };
        display_format current_format_     {};

        //~ remember windowed state for restore
        RECT          windowed_rect_       {};
        LONG_PTR      windowed_style_      { 0 };

        bool          tearing_supported_   { false };
        bool          is_occluded_         { false };
    };
} // namespace cots::graphics::hardware

#endif
