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
#ifndef CURSEOFTHESEA_DISPLAY_H
#define CURSEOFTHESEA_DISPLAY_H

#include <cstdint>
#include <string>
#include <vector>

namespace trishul::render
{
    //~ one supported video mode on a monitor
    struct display_mode
    {
        std::uint32_t width              { 0u };
        std::uint32_t height             { 0u };
        std::uint32_t refresh_numerator  { 0u };
        std::uint32_t refresh_denominator{ 0u };

        [[nodiscard]] float refresh_hz() const noexcept
        {
            return refresh_denominator
                ? static_cast<float>(refresh_numerator) /
                  static_cast<float>(refresh_denominator) : 0.f;
        }
    };

    //~ one selectable gpu
    struct adapter_option
    {
        std::uint32_t index                 { 0u }; //~ feed back to select it
        std::string   name;
        std::uint64_t dedicated_video_memory{ 0u };
        std::uint32_t vendor_id             { 0u };
        std::uint32_t device_id             { 0u };
        bool          is_warp               { false };
    };

    //~ one selectable monitor with its mode list
    struct output_option
    {
        std::uint32_t index         { 0u };
        std::string   name;
        std::uint32_t desktop_width { 0u };
        std::uint32_t desktop_height{ 0u };
        bool          is_primary    { false };
        std::vector<display_mode> modes;
    };

    //~ everything a settings menu ever needs
    struct display_capabilities
    {
        std::uint32_t current_adapter_index{ 0u };
        std::vector<adapter_option> adapters;
        std::vector<output_option>  outputs;
    };

    enum class window_mode : std::uint8_t
    {
        windowed,
        borderless,
        exclusive
    };

    //~ whatever user picker
    struct display_settings
    {
        bool          manual_adapter     { false }; //~ false keeps the auto pick
        std::uint32_t adapter_index      { 0u };
        std::uint32_t output_index       { 0u };    //~ which monitor
        std::uint32_t width              { 0u };    //~ 0 use the native desktop
        std::uint32_t height             { 0u };
        std::uint32_t refresh_numerator  { 0u };    //~ 0 use default
        std::uint32_t refresh_denominator{ 0u };
        window_mode   mode               { window_mode::windowed };
        bool          vsync              { true };
    };

    //~ pretty vendor label
    [[nodiscard]] inline const char* vendor_name(const std::uint32_t vendor_id) noexcept
    {
        switch (vendor_id)
        {
        case 0x10DEu: return "NVIDIA";
        case 0x1002u: return "AMD";
        case 0x8086u: return "Intel";
        case 0x1414u: return "Microsoft";
        default:      return "Unknown";
        }
    }
} // namespace trishul::render

#endif //CURSEOFTHESEA_DISPLAY_H