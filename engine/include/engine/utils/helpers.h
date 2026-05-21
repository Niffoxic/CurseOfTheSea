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
} // namespace cots::helpers

#endif //CURSEOFTHESEA_HELPERS_H
