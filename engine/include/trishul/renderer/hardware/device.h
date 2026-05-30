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
#ifndef CURSEOFTHESEA_DEVICE_H
#define CURSEOFTHESEA_DEVICE_H

#include "trishul/core/engine_config.h"

#include <cstdint>
#include <string>
#include <vector>
#include <wrl/client.h>

//~ don't care about heavy copying its only being used in renderer which
//~ itself is an impl idiom
#include <d3d12.h>
#include <dxgi1_6.h>

namespace D3D12MA { class Allocator; }

namespace trishul::render::hardware
{
    struct adapter_info
    {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
        std::string   name;
        std::uint32_t adapter_index         { config::INVALID_INDEX };
        std::uint32_t vendor_id             { 0u };
        std::uint32_t device_id             { 0u };
        std::uint64_t dedicated_video_memory{ 0u };
        std::uint64_t shared_memory         { 0u };
        bool          is_wrap               { false };

        //~ comptr already refcounts just drop our reference
        void release() noexcept
        {
            adapter.Reset();
        }
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
        std::uint32_t  index         { 0u };
        std::string    device_name;
        std::int32_t   desktop_left  { 0 };
        std::int32_t   desktop_top   { 0 };
        std::uint32_t  desktop_width { 0u };
        std::uint32_t  desktop_height{ 0u };
        bool           is_primary    { false };
        display_format native_mode;
        std::vector<display_format> supported_modes;
    };

    struct device_create_info
    {
        bool manual = false; //~ manually target a specific adapter_index
        DXGI_GPU_PREFERENCE preference          { DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE };
        std::uint32_t       adapter_index       { 0 };
        bool                allow_warp_fallback { true };

        //~ adapters carrying any of these flags are excluded from auto pick
        UINT flags = DXGI_ADAPTER_FLAG_SOFTWARE;

        //~ floor for selection real caps are checked after create
        D3D_FEATURE_LEVEL min_feature_level { D3D_FEATURE_LEVEL_12_0 };
    };

    class device final
    {
    public:
         device() = default;
        ~device();

        device(const device&) noexcept = delete;
        device(device&&)      noexcept = delete;

        device& operator=(const device&) noexcept = delete;
        device& operator=(device&&)      noexcept = delete;

        //~ lifecycle
        [[nodiscard]]
        bool initialize  (const device_create_info& info = {});
        void deinitialize() noexcept;

        //~ flag to recreate if d3d12 driver corrupted (dont recall the precise error
        //~ but thats annoying for debug and also using this for fullscreen swaps)
        [[nodiscard]] bool recreate(const device_create_info& info) noexcept;
        [[nodiscard]] bool recreate(); //~ uses the last info

        [[nodiscard]] bool check_device_removed() const;
                      void refresh_adapters    ();

        //~ dump dred breadcrumbs on device removed and page fault info and reason
        void dump_device_removed() const;

        //~ getters
        [[nodiscard]] ID3D12Device14*     d3d12_device  () const noexcept;
        [[nodiscard]] IDXGIFactory7*      dxgi_factory  () const noexcept;
        [[nodiscard]] ID3D12CommandQueue* graphics_queue() const noexcept;
        [[nodiscard]] ID3D12CommandQueue* compute_queue () const noexcept;
        [[nodiscard]] ID3D12CommandQueue* copy_queue    () const noexcept;
        [[nodiscard]] D3D12MA::Allocator* allocator     () const noexcept;
        [[nodiscard]] D3D_FEATURE_LEVEL   feature_level () const noexcept;

        [[nodiscard]] const adapter_info&              current_adapter_info() const noexcept;
        [[nodiscard]] const std::vector<adapter_info>& adapters_info       () const noexcept;
        [[nodiscard]] bool                             is_initialized      () const noexcept;

        [[nodiscard]] const std::vector<output_info>& outputs() const noexcept
        {
            return outputs_info_;
        }
        void refresh_outputs();

    private:
        //~ result of choosing a physical adapter
        struct adapter_selection
        {
            Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter;
            std::uint32_t     adapter_index { config::INVALID_INDEX };
            D3D_FEATURE_LEVEL feature_level { D3D_FEATURE_LEVEL_12_0 };
            bool              is_warp       { false };
        };

        bool create_internal(const device_create_info& info);
        bool pick_adapter   (const device_create_info& info,
                             adapter_selection& out) const;

        [[nodiscard]] bool verify_device_features() const;
        [[nodiscard]] bool create_command_queues ();
        [[nodiscard]] bool create_allocator      ();

        void destroy_internal  () noexcept;
        void enumerate_outputs ();

        void enumerate_adapters(
            DXGI_GPU_PREFERENCE pref,
            std::vector<adapter_info>& out,
            UINT flags = 0u
        ) const;

        static adapter_info describe_adapter(
            const DXGI_ADAPTER_DESC1& desc, std::uint32_t index);

    private:
        Microsoft::WRL::ComPtr<IDXGIFactory7>      factory_;
        Microsoft::WRL::ComPtr<IDXGIAdapter4>      adapter_;
        Microsoft::WRL::ComPtr<ID3D12Device14>     device_;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> graphics_queue_;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> compute_queue_;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> copy_queue_;
        Microsoft::WRL::ComPtr<ID3D12InfoQueue1>   info_queue_;

        D3D12MA::Allocator*       allocator_ { nullptr };
        std::vector<adapter_info> adapters_info_;
        std::vector<output_info>  outputs_info_;

        adapter_info       adapter_info_{};
        device_create_info last_info_   {};
        D3D_FEATURE_LEVEL  feature_level_{ D3D_FEATURE_LEVEL_12_0 };

        //~ status info
        bool initialized_           { false };
        std::uint32_t created_times_{ 0u }; //~ to debug aggresive creation
    };

} // namespace trishul::render

#endif //CURSEOFTHESEA_DEVICE_H