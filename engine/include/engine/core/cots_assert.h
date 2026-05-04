// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_COTS_ASSERT_H
#define CURSEOFTHESEA_COTS_ASSERT_H

#include <fstream>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <string>
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
        const auto now = std::time(nullptr);
        std::tm tm_buf;
        localtime_s(&tm_buf, &now);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
        return buf;
    }

    inline std::string build_line(const char* expr, const char* file, const int line,
                                  const char* func, const char* msg)
    {
        char buf[2048];
        std::snprintf(buf, sizeof(buf),
            "%s [ASSERT] %s:%d in %s(): expression `%s` failed%s%s",
            current_timestamp().c_str(),
            file, line, func, expr,
            (msg && msg[0]) ? " — " : "",
            (msg && msg[0]) ? msg   : "");
        return buf;
    }

    [[noreturn]] inline void handle_failure(const char* expr, const char* file,
                                            const int line, const char* func,
                                            const char* msg)
    {
        const std::string text = build_line(expr, file, line, func, msg);

        {
            std::ofstream f("error.log", std::ios::app);
            if (f.is_open()) f << text << std::endl;
        }

        std::fputs(text.c_str(), stderr);
        std::fputc('\n', stderr);
        std::fflush(stderr);

        char clickable[2048];
        std::snprintf(clickable, sizeof(clickable),
                      "%s(%d): %s\n", file, line, text.c_str());
        OutputDebugStringA(clickable);

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
} // namespace cots::detail

#define COTS_INTERNAL_FAIL(expr, msg) do {\
if (!(expr)){\
::cots::detail::handle_failure(\
#expr, __FILE__, __LINE__, __func__, msg);\
}}while(0)

//~ always on
#define COTS_VERIFY(expr)          COTS_INTERNAL_FAIL(expr, nullptr)
#define COTS_VERIFY_MSG(expr, msg) COTS_INTERNAL_FAIL(expr, msg)

//~ only on debug or tracer
#if defined(DEBUG) || defined(_DEBUG) || defined(TRACER)
#  define COTS_ASSERT(expr)          COTS_INTERNAL_FAIL(expr, nullptr)
#  define COTS_ASSERT_MSG(expr, msg) COTS_INTERNAL_FAIL(expr, msg)
#else
#  define COTS_ASSERT(expr)          ((void)0)
#  define COTS_ASSERT_MSG(expr, msg) ((void)0)
#endif

#endif //CURSEOFTHESEA_COTS_ASSERT_H
