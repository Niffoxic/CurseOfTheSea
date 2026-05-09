// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_HELPERS_H
#define CURSEOFTHESEA_HELPERS_H

#include <string>
#include <windows.h>

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
} // namespace cots::helpers

#endif //CURSEOFTHESEA_HELPERS_H
