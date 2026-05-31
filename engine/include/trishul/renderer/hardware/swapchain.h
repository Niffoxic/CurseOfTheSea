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
#ifndef CURSEOFTHESEA_SWAPCHAIN_H
#define CURSEOFTHESEA_SWAPCHAIN_H

#include <array>
#include <atomic>
#include <cstdint>
#include <wrl/client.h>
#include <windows.h>

#include "types.h"
#include "device.h"
#include "trishul/core/engine_config.h"

struct IDXGISwapChain4;
struct IDXGIOutput6;
struct ID3D12Resource2;
struct ID3D12DescriptorHeap;

namespace trishul::render::hardware
{
    //~ present outcome the render loop branches on this to detect device
    // removed and to skip rendering while occluded
    enum class present_result : std::uint8_t
    {
        success,
        occluded,
        device_removed,
        failed,
    };

    struct swapchain_create_info
    {
        HWND           window_handle { nullptr };
        std::uint32_t  width         { 1280 };
        std::uint32_t  height        { 720  };
        display_mode   mode          { display_mode::windowed };
        std::uint32_t  frame_count   { config::SWAPCHAIN_BUFFER_COUNT };
        bool           allow_tearing { true };
        bool           hdr           { false };
        std::uint32_t  output_index  { 0 };  //~ for exclusive fullscreen
        display_format exclusive_mode{};
    };
    
    struct swapchain_config
    {
        const device*         dev { nullptr };
        swapchain_create_info info{};
    };

    class swapchain final: public interfaces
    {
    public:
         swapchain() = default;
        ~swapchain() override;

        swapchain(const swapchain&) = delete;
        swapchain(swapchain&&)      = delete;

        swapchain& operator=(const swapchain&) = delete;
        swapchain& operator=(swapchain&&)      = delete;

        //~ lifecycle
        [[nodiscard]]
        bool initialize  ()          override;
        void deinitialize() noexcept override;

        [[nodiscard]] bool need_rebuild() const noexcept override
        {
            return need_rebuild_.load(std::memory_order_acquire);
        }
        [[nodiscard]] const char* name() const noexcept override { return "swapchain"; }

        //~ flag a rebuild
        void mark_for_rebuild() noexcept
        {
            need_rebuild_.store(true, std::memory_order_release);
        }

        //~ fast path just resize backbuffers keep mode and format
        [[nodiscard]] bool resize(std::uint32_t width, std::uint32_t height);

        //~ full recreate new params full teardown
        [[nodiscard]] bool recreate(const swapchain_create_info& info);

        //~ mode transitions
        [[nodiscard]] bool set_display_mode  (display_mode mode);
        [[nodiscard]] bool set_exclusive_mode(std::uint32_t output_index,
                                              const display_format& format);
        [[nodiscard]] bool set_windowed_size (std::uint32_t width, std::uint32_t height);

        //~ per frame returns a typed result device removed is surfaced to
        // the caller for dred dump and clean exit
        present_result present(std::uint32_t sync_interval = 0);

        //~ occlusion call once per frame at start returns true if rendering
        //  should proceed false if occluded
        [[nodiscard]] bool check_occlusion();

        //~ accessors
        [[nodiscard]] std::uint32_t      current_backbuffer_index() const;
        [[nodiscard]] ID3D12Resource2*   current_backbuffer      () const;
        [[nodiscard]] std::size_t        current_rtv_handle      () const;
        [[nodiscard]] IDXGISwapChain4*   dxgi_swapchain          () const noexcept;

        [[nodiscard]] std::uint32_t      width               () const noexcept { return width_;                 }
        [[nodiscard]] std::uint32_t      height              () const noexcept { return height_;                }
        [[nodiscard]] display_mode       current_mode        () const noexcept { return current_mode_;          }
        [[nodiscard]] const display_format& current_format   () const noexcept { return current_format_;        }
        [[nodiscard]] std::uint32_t      current_output_index() const noexcept { return current_output_index_;  }
        [[nodiscard]] bool               is_occluded         () const noexcept { return is_occluded_;           }
        [[nodiscard]] bool               tearing_supported   () const noexcept { return tearing_supported_;     }

    private:
        bool build();  //~ (re)create on device with create_info

        bool create_swapchain       (const swapchain_create_info& info);
        bool create_backbuffer_views();
        void release_backbuffers    () noexcept;

        bool apply_borderless   (const swapchain_create_info& info);
        bool apply_exclusive    (const swapchain_create_info& info);
        bool apply_windowed     (const swapchain_create_info& info);

        bool find_output        (std::uint32_t index,
                                 Microsoft::WRL::ComPtr<IDXGIOutput6>& out) const;

        static display_format pick_closest_mode(const output_info& out,
                                                const display_format& requested);

    private:
        const device*         device_     { nullptr };
        swapchain_create_info create_info_{};

        Microsoft::WRL::ComPtr<IDXGISwapChain4>      swapchain_;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_heap_;
        std::array<Microsoft::WRL::ComPtr<ID3D12Resource2>, config::MAX_BACKBUFFER_COUNT> backbuffers_;

        HWND          window_handle_       { nullptr };
        std::uint32_t width_               { 1 };
        std::uint32_t height_              { 1 };
        std::uint32_t frame_count_         { config::SWAPCHAIN_BUFFER_COUNT };
        std::uint32_t rtv_descriptor_size_ { 0 };
        std::uint32_t current_output_index_{ 0 };

        display_mode   current_mode_       { display_mode::windowed };
        display_format current_format_     {};

        //~ remember windowed state for restore
        RECT          windowed_rect_       {};
        LONG_PTR      windowed_style_      { 0 };

        bool              tearing_supported_{ false };
        bool              is_occluded_      { false };
        std::atomic<bool> need_rebuild_     { true };
    };
} // namespace trishul::render::hardware

#endif //CURSEOFTHESEA_SWAPCHAIN_H
