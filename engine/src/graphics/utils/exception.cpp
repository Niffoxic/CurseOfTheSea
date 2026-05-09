// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/utils/exception.h"

#include <comdef.h>
#include <format>
#include <windows.h>

namespace cots::graphics::hardware
{
    namespace
    {
        std::string wide_to_utf8(const wchar_t* w)
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

        std::string format_hresult(const HRESULT hr)
        {
            const _com_error err(hr);
            std::string msg = wide_to_utf8(err.ErrorMessage());
            if (msg.empty()) msg = "Unknown DirectX error.";
            return msg;
        }
    }

    exception::exception(
        const char*      file,
        const int        line,
        const char*      function,
        const HRESULT    hr,
        std::string_view context) noexcept
        : file_          (file ? file : "<unknown>")
        , function_      (function ? function : "<unknown>")
        , context_       (context)
        , system_message_(format_hresult(hr))
        , line_          (line)
        , hresult_       (hr)
    {
        try
        {
            if (context_.empty())
            {
                full_message_ = std::format(
                    "[DxException] {}\n"
                    "  HRESULT : 0x{:08X}\n"
                    "  File    : {}\n"
                    "  Line    : {}\n"
                    "  Function: {}",
                    system_message_,
                    static_cast<std::uint32_t>(hresult_),
                    file_, line_, function_);
            }
            else
            {
                full_message_ = std::format(
                    "[DxException] {} ({})\n"
                    "  HRESULT : 0x{:08X}\n"
                    "  File    : {}\n"
                    "  Line    : {}\n"
                    "  Function: {}",
                    context_, system_message_,
                    static_cast<std::uint32_t>(hresult_),
                    file_, line_, function_);
            }
        }
        catch (...)
        {
            full_message_ = "[DxException] (message formatting failed)";
        }
    }

    const char* exception::what() const noexcept
    {
        return full_message_.c_str();
    }
}
