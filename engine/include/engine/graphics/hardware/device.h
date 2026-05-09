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

namespace cots::graphics::hardware
{
    struct adapter_info
    {
        std::string   name;
        std::uint32_t adapter_index;
        std::uint32_t vendor_id;
        std::uint32_t device_id;
        std::uint64_t dedicated_video_memory;
        std::uint64_t shared_memory;
        bool          is_wrap;
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

    private:
        bool create_internal (const device_create_info& info);
        bool pick_adapter    (const device_create_info& info,
                              Microsoft::WRL::ComPtr<IDXGIAdapter4>& out) const;

        void destroy_internal  () noexcept;
        void enumerate_adapters();

    private:
        Microsoft::WRL::ComPtr<IDXGIFactory7>      factory_;
        Microsoft::WRL::ComPtr<IDXGIAdapter4>      adapter_;
        Microsoft::WRL::ComPtr<ID3D12Device14>     device_;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> graphics_queue_;
        Microsoft::WRL::ComPtr<ID3D12InfoQueue1>   info_queue_;

        std::vector<adapter_info>   adapters_info_;
        adapter_info                adapter_info_ {};
        device_create_info          last_info_    {};
        bool                        initialized_  { false };
    };
}

#endif
