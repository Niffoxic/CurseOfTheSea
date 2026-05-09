// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_RHI_EXCEPTION_H
#define CURSEOFTHESEA_RHI_EXCEPTION_H

#include <exception>
#include <string>
#include <string_view>
#include <winerror.h>

namespace cots::graphics::hardware
{
    class exception final : public std::exception
    {
    public:
        exception(
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

} // namespace cots::graphics::hardware

#define COTS_DX_THROW_IF_FAILED(expr)                                     \
    do {                                                                  \
        const HRESULT cots_hr = (expr);                                   \
        if (FAILED(cots_hr)) {                                            \
            throw ::cots::graphics::hardware::exception(                  \
                __FILE__, __LINE__, __func__, cots_hr);                   \
        }                                                                 \
    } while (0)

#define COTS_DX_THROW_IF_FAILED_MSG(expr, msg)                            \
    do {                                                                  \
        const HRESULT cots_hr = (expr);                                   \
        if (FAILED(cots_hr)) {                                            \
            throw ::cots::graphics::hardware::exception(                  \
                __FILE__, __LINE__, __func__, cots_hr, (msg));            \
        }                                                                 \
    } while (0)

#endif
