#ifndef CURSEOFTHESEA_DEVICE_H
#define CURSEOFTHESEA_DEVICE_H

#include <cstdint>
#include <string>
#include <vector>
#include <wrl/client.h>
#include "types.h"

struct ID3D12Device14;
struct IDXGIFactory7;
struct IDXGIAdapter4;
struct ID3D12CommandQueue;
struct ID3D12InfoQueue1;
struct IDXGIAdapter1;

namespace cots::graphics::hardware
{
    struct adapter_info
    {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
        std::string   name;
        std::uint32_t adapter_index;
        std::uint32_t vendor_id;
        std::uint32_t device_id;
        std::uint64_t dedicated_video_memory;
        std::uint64_t shared_memory;
        bool          is_wrap;
    };

    struct display_format
    {
        std::uint32_t width              { 0u };
        std::uint32_t height             { 0u };
        std::uint32_t refresh_numerator  { 0u };
        std::uint32_t refresh_denominator{ 0u };

        [[nodiscard]]
        float refresh_hz() const noexcept
        {
            return refresh_denominator > 0
                ?   static_cast<float>(refresh_numerator) /
                    static_cast<float>(refresh_denominator) : 0.f;
        }
    };

    struct output_info
    {
        std::uint32_t  index;
        std::string    device_name;
        std::int32_t   desktop_left;
        std::int32_t   desktop_top;
        std::uint32_t  desktop_width;
        std::uint32_t  desktop_height;
        bool           is_primary;
        display_format native_mode;
        std::vector<display_format> supported_modes;
    };

    enum class adapter_preference : std::uint8_t
    {
        high_performance = 0,
        low_power,
        manual
    };

    struct device_create_info
    {
        adapter_preference preference          { adapter_preference::high_performance };
        std::uint32_t      adapter_index       { 0 };
        bool               allow_warp_fallback { true };
    };

    class device final
    {
    public:
         device();
        ~device();

        device(const device&) = delete;
        device(device&&)      = delete;

        device& operator=(const device&) = delete;
        device& operator=(device&&)      = delete;

        [[nodiscard]] bool initialize  (const device_create_info& info = {});
                      void deinitialize() noexcept;

        [[nodiscard]] bool recreate(const device_create_info& info);
        [[nodiscard]] bool recreate();   //~ uses last info

        [[nodiscard]] bool check_device_removed() const;
                      void refresh_adapters    ();

        [[nodiscard]] ID3D12Device14*     d3d12_device  () const noexcept;
        [[nodiscard]] IDXGIFactory7*      dxgi_factory  () const noexcept;
        [[nodiscard]] ID3D12CommandQueue* graphics_queue() const noexcept;

        [[nodiscard]] const adapter_info&              current_adapter_info() const noexcept;
        [[nodiscard]] const std::vector<adapter_info>& adapters_info       () const noexcept;
        [[nodiscard]] bool                             is_initialized      () const noexcept;

        [[nodiscard]] const std::vector<output_info>& outputs() const noexcept
        {
            return outputs_info_;
        }
        void refresh_outputs();

    private:
        bool create_internal (const device_create_info& info);
        bool pick_adapter    (const device_create_info& info,
                              Microsoft::WRL::ComPtr<IDXGIAdapter4>& out) const;

        void destroy_internal  () noexcept;
        void enumerate_adapters();
        void enumerate_outputs ();

    private:
        Microsoft::WRL::ComPtr<IDXGIFactory7>      factory_;
        Microsoft::WRL::ComPtr<IDXGIAdapter4>      adapter_;
        Microsoft::WRL::ComPtr<ID3D12Device14>     device_;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> graphics_queue_;
        Microsoft::WRL::ComPtr<ID3D12InfoQueue1>   info_queue_;

        std::vector<adapter_info>   adapters_info_;
        std::vector<output_info>    outputs_info_;

        adapter_info                adapter_info_{};
        device_create_info          last_info_   {};

        bool                        initialized_ { false };
    };
}

#endif
