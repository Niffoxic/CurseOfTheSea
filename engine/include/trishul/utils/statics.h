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
#ifndef CURSEOFTHESEA_STATICS_H
#define CURSEOFTHESEA_STATICS_H

#include <comdef.h>
#include <format>
#include <windows.h>

namespace statics //~ niffoxic cross project helpers
{
    __forceinline static std::string wide_to_utf8(const wchar_t* w)
    {
        if (!w || !*w) return {};

        const int len = WideCharToMultiByte(CP_UTF8,
            0, w, -1,
            nullptr, 0,
            nullptr, nullptr);
        if (len <= 1) return {};

        std::string out(static_cast<std::size_t>(len - 1), '\0');
        WideCharToMultiByte(CP_UTF8, 0, w,
            -1, out.data(),
            len, nullptr,
            nullptr
            );
        return out;
    }

     __forceinline static std::string format_hresult(const HRESULT hr)
    {
        const _com_error err(hr);
        std::string msg = wide_to_utf8(err.ErrorMessage());
        if (msg.empty()) msg = "Unknown DirectX error.";
        return msg;
    }
} // statics

#endif //CURSEOFTHESEA_STATICS_H
