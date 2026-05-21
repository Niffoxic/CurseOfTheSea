// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_HELPERS_H
#define CURSEOFTHESEA_HELPERS_H

#include <string>
#include <windows.h>
#include <d3dx12/d3dx12.h>

#include "engine/graphics/hardware/types.h"

namespace cots::helpers
{
    inline std::string wide_to_utf8(const wchar_t* wide)
    {
        if (not wide || !*wide) return {};

        const int len = WideCharToMultiByte(
            CP_UTF8,
            0, wide,
            -1,
            nullptr, 0,
            nullptr, nullptr
        );

        if (len <= 1) return {};

        std::string out(static_cast<std::size_t>(len - 1), '\0');

        WideCharToMultiByte(
            CP_UTF8,
            0, wide,
            -1, out.data(),
            len, nullptr,
            nullptr
        );

        return out;
    }

    inline std::wstring to_wide(const std::string_view s)
    {
        if (s.empty()) return {};
        const int len = MultiByteToWideChar(CP_UTF8, 0, s.data(),
                                            static_cast<int>(s.size()), nullptr, 0);
        std::wstring out(static_cast<std::size_t>(len), L'\0');
        MultiByteToWideChar(
            CP_UTF8,
            0,
            s.data(),
            static_cast<int>(s.size()),
                            out.data(),
                            len
        );
        return out;
    }

    inline D3D12_COMMAND_LIST_TYPE to_d3d12(graphics::hardware::command_list_type t)
    {
        using graphics::hardware::command_list_type;
        switch (t)
        {
            case command_list_type::compute: return D3D12_COMMAND_LIST_TYPE_COMPUTE;
            case command_list_type::copy:    return D3D12_COMMAND_LIST_TYPE_COPY;
            case command_list_type::direct:
            default:                         return D3D12_COMMAND_LIST_TYPE_DIRECT;
        }
    }

    inline D3D12_RESOURCE_STATES to_d3d12(graphics::hardware::resource_state s)
    {
        using graphics::hardware::resource_state;
        switch (s)
        {
            case resource_state::render_target: return D3D12_RESOURCE_STATE_RENDER_TARGET;
            case resource_state::present:       return D3D12_RESOURCE_STATE_PRESENT;
            case resource_state::depth_write:   return D3D12_RESOURCE_STATE_DEPTH_WRITE;
            case resource_state::depth_read:    return D3D12_RESOURCE_STATE_DEPTH_READ;
            case resource_state::shader_read:   return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            case resource_state::copy_source:   return D3D12_RESOURCE_STATE_COPY_SOURCE;
            case resource_state::copy_dest:     return D3D12_RESOURCE_STATE_COPY_DEST;
            case resource_state::unordered:     return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            case resource_state::common:
            default:                            return D3D12_RESOURCE_STATE_COMMON;
        }
    }

    namespace markers
    {
        constexpr std::uint64_t frame    = 0xFF50A0FF; //~ blue
        constexpr std::uint64_t record   = 0xFF40C040; //~ green  (GPU timeline)
        constexpr std::uint64_t submit   = 0xFFFFA000; //~ orange
        constexpr std::uint64_t present  = 0xFFB060FF; //~ purple
        constexpr std::uint64_t command  = 0xFFFF4040; //~ red    (swapchain changes)
    }

    //~ byte helpers
    constexpr char k_b64[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    inline std::string b64_encode(const std::vector<std::uint8_t>& in)
    {
        std::string out;
        out.reserve(((in.size() + 2) / 3) * 4);
        std::size_t i = 0;
        while (i + 3 <= in.size())
        {
            const std::uint32_t n = (in[i] << 16) | (in[i+1] << 8) | in[i+2];
            out += k_b64[(n >> 18) & 63]; out += k_b64[(n >> 12) & 63];
            out += k_b64[(n >> 6) & 63];  out += k_b64[n & 63];
            i += 3;
        }
        if (const std::size_t rem = in.size() - i; rem == 1)
        {
            const std::uint32_t n = in[i] << 16;
            out += k_b64[(n >> 18) & 63]; out += k_b64[(n >> 12) & 63];
            out += "==";
        }
        else if (rem == 2)
        {
            const std::uint32_t n = (in[i] << 16) | (in[i+1] << 8);
            out += k_b64[(n >> 18) & 63]; out += k_b64[(n >> 12) & 63];
            out += k_b64[(n >> 6) & 63];  out += '=';
        }
        return out;
    }

    inline std::vector<std::uint8_t> b64_decode(const std::string& s)
    {
        auto val = [](char c) -> int {
            if (c >= 'A' && c <= 'Z') return c - 'A';
            if (c >= 'a' && c <= 'z') return c - 'a' + 26;
            if (c >= '0' && c <= '9') return c - '0' + 52;
            if (c == '+') return 62;
            if (c == '/') return 63;
            return -1;
        };
        std::vector<std::uint8_t> out;
        std::uint32_t buf = 0; int bits = 0;
        for (const char c : s)
        {
            const int v = val(c);
            if (v < 0) continue;
            buf = (buf << 6) | static_cast<std::uint32_t>(v);
            bits += 6;
            if (bits >= 8) { bits -= 8; out.push_back(static_cast<std::uint8_t>((buf >> bits) & 0xFF)); }
        }
        return out;
    }
} // namespace cots::helpers

#endif //CURSEOFTHESEA_HELPERS_H

