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

#include <cstdint>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

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

    __forceinline static std::wstring to_wide(const std::string_view s)
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

    //~ rounded up to 256
     __forceinline static std::uint64_t align_cb_size(const std::uint64_t size, const bool is_constant) noexcept
    {
        if (!is_constant) return size;
        return (size + 255ull) & ~255ull;
    }

    //~ slurping a whole file into a string opening binary so we get every byte
    //~ as is returns false when the file will not open caller decides what next
    inline bool read_file(const std::string_view path, std::string& out)
    {
        std::ifstream f(std::string(path), std::ios::binary | std::ios::ate);
        if (!f.is_open()) return false;

        const std::streamsize size = f.tellg();
        if (size < 0) return false;

        f.seekg(0, std::ios::beg);
        out.resize(static_cast<std::size_t>(size));
        if (size > 0 && !f.read(out.data(), size)) return false;
        return true;
    }

    //~ tagging the build kind so a debug and a release shader never share a
    //~ cache slot keeping it self contained off the standard debug macro
    inline const char* cfg_tag() noexcept
    {
#if defined(_DEBUG)
        return "dbg";
#else
        return "rel";
#endif
    }

    //~ hashing bytes the fnv1a way cheap stable and good enough for a cache key
    inline std::uint64_t fnv1a(const std::string_view data) noexcept
    {
        std::uint64_t h = 14695981039346656037ull; //~ offset basis
        for (const unsigned char c : data)
        {
            h ^= c;
            h *= 1099511628211ull;               //~ fnv prime
        }
        return h;
    }

    //~ packing bytes into base64 so a blob like dxil can ride inside json three
    //~ bytes become four chars padding with = when the tail does not line up
    inline std::string b64_encode(const std::span<const std::uint8_t> data)
    {
        static constexpr char tbl[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string out;
        out.reserve(((data.size() + 2u) / 3u) * 4u);

        std::size_t i = 0;
        for (; i + 3u <= data.size(); i += 3u)
        {
            const std::uint32_t n = (static_cast<std::uint32_t>(data[i])     << 16)
                                  | (static_cast<std::uint32_t>(data[i + 1]) << 8)
                                  |  static_cast<std::uint32_t>(data[i + 2]);
            out.push_back(tbl[(n >> 18) & 0x3F]);
            out.push_back(tbl[(n >> 12) & 0x3F]);
            out.push_back(tbl[(n >>  6) & 0x3F]);
            out.push_back(tbl[ n        & 0x3F]);
        }

        //~ mopping up the last one or two bytes with = padding
        if (const std::size_t rem = data.size() - i; rem == 1u)
        {
            const std::uint32_t n = static_cast<std::uint32_t>(data[i]) << 16;
            out.push_back(tbl[(n >> 18) & 0x3F]);
            out.push_back(tbl[(n >> 12) & 0x3F]);
            out.push_back('=');
            out.push_back('=');
        }
        else if (rem == 2u)
        {
            const std::uint32_t n = (static_cast<std::uint32_t>(data[i])     << 16)
                                  | (static_cast<std::uint32_t>(data[i + 1]) << 8);
            out.push_back(tbl[(n >> 18) & 0x3F]);
            out.push_back(tbl[(n >> 12) & 0x3F]);
            out.push_back(tbl[(n >>  6) & 0x3F]);
            out.push_back('=');
        }
        return out;
    }

    //~ unpacking base64 back into bytes skipping padding and any stray junk
    //~ masking the carry each step so a long blob never overflows the accumulator
    inline std::vector<std::uint8_t> b64_decode(const std::string_view s)
    {
        auto sextet = [](const char c) -> int
        {
            if (c >= 'A' && c <= 'Z') return c - 'A';
            if (c >= 'a' && c <= 'z') return c - 'a' + 26;
            if (c >= '0' && c <= '9') return c - '0' + 52;
            if (c == '+') return 62;
            if (c == '/') return 63;
            return -1; //~ = padding or whitespace just skip it
        };

        std::vector<std::uint8_t> out;
        out.reserve((s.size() / 4u) * 3u);

        std::uint32_t buf  = 0u;
        int           bits = 0;
        for (const char c : s)
        {
            const int v = sextet(c);
            if (v < 0) continue;

            buf = (buf << 6) | static_cast<std::uint32_t>(v);
            bits += 6;
            if (bits >= 8)
            {
                bits -= 8;
                out.push_back(static_cast<std::uint8_t>((buf >> bits) & 0xFFu));
                buf &= (1u << bits) - 1u; //~ keep only the leftover bits
            }
        }
        return out;
    }
} // statics

#endif //CURSEOFTHESEA_STATICS_H
