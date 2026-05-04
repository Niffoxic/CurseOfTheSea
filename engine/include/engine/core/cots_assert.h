// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_COTS_ASSERT_H
#define CURSEOFTHESEA_COTS_ASSERT_H

#include <chrono>
#include <format>
#include <fstream>
#include <print>
#include <string>
#include <utility>
#include <cstdlib>
#include <windows.h>
#include <intrin.h>

#if (defined(DEBUG) || defined(_DEBUG))
#  define _CRTDBG_MAP_ALLOC
#  include <stdlib.h>
#  include <crtdbg.h>
#  define COTS_HAS_CRTDBG 1
#else
#  define COTS_HAS_CRTDBG 0
#endif

namespace cots
{
    inline void init_debug_runtime(const bool enable_leak_check = true)
    {
    #if COTS_HAS_CRTDBG
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_WNDW);
        _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode(_CRT_ERROR,  _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_WNDW);
        _CrtSetReportFile(_CRT_ERROR,  _CRTDBG_FILE_STDERR);
        _CrtSetReportMode(_CRT_WARN,   _CRTDBG_MODE_DEBUG);

        if (enable_leak_check)
        {
            int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
            flags |= _CRTDBG_ALLOC_MEM_DF;     // track allocations
            flags |= _CRTDBG_LEAK_CHECK_DF;    // dump leaks at exit
            flags |= _CRTDBG_CHECK_ALWAYS_DF;  // heap check on every alloc
            _CrtSetDbgFlag(flags);
        }
    #else
        (void)enable_leak_check;
    #endif
    }
}

namespace cots::detail
{
    inline std::string current_timestamp()
    {
        const auto now = std::chrono::floor<std::chrono::seconds>(
            std::chrono::system_clock::now());
        return std::format("{:%Y-%m-%d %H:%M:%S}",
            std::chrono::current_zone()->to_local(now));
    }

    inline std::string build_line(const char* expr, const char* file, const int line,
                                  const char* func, const char* msg)
    {
        return std::format(
            "{} [ASSERT] {}:{} in {}(): expression `{}` failed{}{}",
            current_timestamp(),
            file, line, func, expr,
            (msg && msg[0]) ? " — " : "",
            (msg && msg[0]) ? msg   : "");
    }

    [[noreturn]] inline void handle_failure(const char* expr, const char* file,
                                            const int line, const char* func,
                                            const char* msg)
    {
        const std::string text = build_line(expr, file, line, func, msg);

        {
            std::ofstream f("error.log", std::ios::app);
            if (f.is_open()) std::println(f, "{}", text);
        }

        std::println(stderr, "{}", text);
        std::fflush(stderr);

        const std::string clickable =
            std::format("{}({}): {}\n", file, line, text);
        OutputDebugStringA(clickable.c_str());

    #if COTS_HAS_CRTDBG
        const int report = _CrtDbgReport(
            _CRT_ASSERT, file, line, func,
            "%s", (msg && msg[0]) ? msg : expr);

        if (report == 1) _CrtDbgBreak();
    #else
        if (IsDebuggerPresent()) __debugbreak();
    #endif

        std::abort();
    }

    template<typename... Args>
    [[noreturn]] inline void handle_failure_fmt(
        const char* expr, const char* file, const int line, const char* func,
        std::format_string<Args...> fmt, Args&&... args)
    {
        const std::string msg = std::format(fmt, std::forward<Args>(args)...);
        handle_failure(expr, file, line, func, msg.c_str());
    }
} // namespace cots::detail

#define COTS_INTERNAL_FAIL(expr) do {\
    if (!(expr)){\
        ::cots::detail::handle_failure(\
            #expr, __FILE__, __LINE__, __func__, nullptr);\
    }}while(0)

#define COTS_INTERNAL_FAIL_FMT(expr, ...) do {\
    if (!(expr)){\
        ::cots::detail::handle_failure_fmt(\
            #expr, __FILE__, __LINE__, __func__, __VA_ARGS__);\
    }}while(0)

//~ always on
#define COTS_VERIFY(expr)              COTS_INTERNAL_FAIL(expr)
#define COTS_VERIFY_MSG(expr, ...)     COTS_INTERNAL_FAIL_FMT(expr, __VA_ARGS__)

//~ only on debug or tracer
#if defined(DEBUG) || defined(_DEBUG) || defined(TRACER)
#  define COTS_ASSERT(expr)            COTS_INTERNAL_FAIL(expr)
#  define COTS_ASSERT_MSG(expr, ...)   COTS_INTERNAL_FAIL_FMT(expr, __VA_ARGS__)
#define COTS_FAIL_MSG(...) \
::cots::detail::handle_failure_fmt(\
"<unconditional fail>", __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
# define COTS_ASSERT(expr)            ((void)0)
# define COTS_ASSERT_MSG(expr, ...)   ((void)0)
# define COTS_FAIL_MSG(...) ((void)0)
#endif

#endif //CURSEOFTHESEA_COTS_ASSERT_H