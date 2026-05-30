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
#ifndef CURSEOFTHESEA_DX_EXCEPTION_H
#define CURSEOFTHESEA_DX_EXCEPTION_H

#include <exception>
#include <string>
#include <string_view>
#include <winerror.h>

namespace trishul::exception
{
    class directx final : public std::exception
    {
    public:
        directx(
            const char*      file,
            int              line,
            const char*      function,
            HRESULT          hr,
            std::string_view context = {}) noexcept;

        [[nodiscard]] const char*        what          () const noexcept override;
        [[nodiscard]] HRESULT            hresult       () const noexcept { return hresult_; }
        [[nodiscard]] const std::string& context       () const noexcept { return context_; }
        [[nodiscard]] const std::string& system_message() const noexcept { return system_message_; }

    private:
        std::string  file_;
        std::string  function_;
        std::string  context_;
        std::string  system_message_;
        std::string  full_message_;
        int          line_;
        HRESULT      hresult_;
    };

} // namespace trishul::exception

#define DX_THROW_IF_FAILED(expr)                                     \
    do {                                                                  \
        const HRESULT engine_hr = (expr);                                   \
        if (FAILED(engine_hr)) {                                            \
            throw ::trishul::exception::directx(                  \
                __FILE__, __LINE__, __func__, engine_hr);                   \
        }                                                                 \
    } while (0)

#define DX_THROW_IF_FAILED_MSG(expr, msg)                            \
    do {                                                                  \
        const HRESULT engine_hr = (expr);                                   \
        if (FAILED(engine_hr)) {                                            \
            throw ::trishul::exception::directx(                  \
                __FILE__, __LINE__, __func__, engine_hr, (msg));            \
        }                                                                 \
    } while (0)

#endif //CURSEOFTHESEA_DX_EXCEPTION_H
