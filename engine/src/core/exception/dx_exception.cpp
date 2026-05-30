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
#include "trishul/core/exception/dx_exception.h"
#include "trishul/utils/statics.h"

trishul::exception::directx::directx(
    const char *file,     const int line,
    const char *function, const HRESULT hr,
    const std::string_view context
) noexcept
    : file_          (file ? file         : "<unknown>")
    , function_      (function ? function : "<unknown>")
    , context_       (context)
    , system_message_(statics::format_hresult(hr))
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

const char* trishul::exception::directx::what() const noexcept
{
    return full_message_.c_str();
}
