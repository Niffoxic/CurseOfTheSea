// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_SWAPCHAIN_H
#define CURSEOFTHESEA_SWAPCHAIN_H

#include <array>
#include <cstdint>
#include "device.h"

struct IDXGISwapChain4;
struct IDXGIOutput6;
struct ID3D12Resource2;

namespace cots::graphics::hardware
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
        std::uint32_t  frame_count   { 3 };
        bool           allow_tearing { true };
        bool           hdr           { false };
        std::uint32_t  output_index  { 0 };  //~ for exclusive fullscreen
        display_format exclusive_mode{};
    };

    class swapchain final
    {
    public:
         swapchain();
        ~swapchain();

        swapchain(const swapchain&) = delete;
        swapchain(swapchain&&)      = delete;

        swapchain& operator=(const swapchain&) = delete;
        swapchain& operator=(swapchain&&)      = delete;

        [[nodiscard]] bool initialize  (const device& dev, const swapchain_create_info& info);
                      void deinitialize() noexcept;

        //~ fast path - just resize backbuffers, keep mode & format
        [[nodiscard]] bool resize(const device& dev, std::uint32_t width, std::uint32_t height);

        //~ full recreate - new params full teardown
        [[nodiscard]] bool recreate(const device& dev, const swapchain_create_info& info);

        //~ mode transitions
        [[nodiscard]] bool set_display_mode      (const device& dev, display_mode mode);
        [[nodiscard]] bool set_exclusive_mode    (const device& dev, std::uint32_t output_index,
                                                  const display_format& format);
        [[nodiscard]] bool set_windowed_size     (const device& dev, std::uint32_t width, std::uint32_t height);

        present_result present(std::uint32_t sync_interval = 0);

        //~ occlusion - call once per frame at start
        [[nodiscard]] bool check_occlusion();

        //~ accessors
        [[nodiscard]] std::uint32_t     current_backbuffer_index() const;
        [[nodiscard]] ID3D12Resource2*  current_backbuffer      () const;
        [[nodiscard]] std::size_t       current_rtv_handle      () const;

        [[nodiscard]] std::uint32_t         width               () const noexcept;
        [[nodiscard]] std::uint32_t         height              () const noexcept;
        [[nodiscard]] display_mode          current_mode        () const noexcept;
        [[nodiscard]] const display_format& current_format      () const noexcept;
        [[nodiscard]] std::uint32_t         current_output_index() const noexcept;
        [[nodiscard]] bool                  is_occluded         () const noexcept;
        [[nodiscard]] bool                  tearing_supported   () const noexcept;
        [[nodiscard]] IDXGISwapChain4*      dxgi_swapchain      () const noexcept;
    private:
        class implementation;
        std::unique_ptr<implementation> impl_;
    };
} // namespace cots::graphics::hardware

#endif
