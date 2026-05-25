// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_LOGGER_H
#define CURSEOFTHESEA_LOGGER_H

#include <windows.h>
#include "engine/core/framework/interface/singleton.h"
#include <spdlog/spdlog.h>

namespace cots::utils
{
    class logger final: public interfaces::singleton<logger>
    {
        COTS_SINGLETON(logger)
         logger() = default;
        ~logger() = default;
    public:

        void initialize()
        {
#if defined(COTS_DEBUG) || defined(COTS_RELWITHDEBINFO)
            if (GetConsoleWindow() != nullptr)
                return;

            if (!AllocConsole())
                return;

            FILE* dummy = nullptr;

            freopen_s(&dummy, "CONOUT$", "w", stdout);
            freopen_s(&dummy, "CONOUT$", "w", stderr);
            freopen_s(&dummy, "CONIN$",  "r", stdin);

            std::ios::sync_with_stdio(true);

            SetConsoleTitleA("Curse of the Sea Debug Console");

            return;
#endif
        }

        void deinitialize()
        {
#if defined(COTS_DEBUG) || defined(COTS_RELWITHDEBINFO)
            if (GetConsoleWindow() != nullptr)
                FreeConsole();
#endif
        }

        template<typename... Args>
        void info(spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            spdlog::info(fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void warn(spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            spdlog::warn(fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void error(spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            spdlog::error(fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void trace(spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            spdlog::trace(fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void debug(spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            spdlog::debug(fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void critical(spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            spdlog::critical(fmt, std::forward<Args>(args)...);
        }

        void set_level(const spdlog::level::level_enum level)
        {
            spdlog::set_level(level);
        }
    };
} // namespace cots::utils

#if defined(COTS_DEBUG) //~ log everything

#define LOG_TRACE(...) ::cots::utils::logger::instance().trace(__VA_ARGS__)
#define LOG_DEBUG(...) ::cots::utils::logger::instance().debug(__VA_ARGS__)
#define LOG_INFO(...) ::cots::utils::logger::instance().info(__VA_ARGS__)
#define LOG_WARN(...) ::cots::utils::logger::instance().warn(__VA_ARGS__)
#define LOG_ERROR(...) ::cots::utils::logger::instance().error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::cots::utils::logger::instance().critical(__VA_ARGS__)

#elif defined(COTS_RELWITHDEBINFO) //~ keeps useful logs

#define LOG_TRACE(...) ((void)0)
#define LOG_DEBUG(...) ::cots::utils::logger::instance().debug(__VA_ARGS__)
#define LOG_INFO(...) ::cots::utils::logger::instance().info(__VA_ARGS__)
#define LOG_WARN(...) ::cots::utils::logger::instance().warn(__VA_ARGS__)
#define LOG_ERROR(...) ::cots::utils::logger::instance().error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::cots::utils::logger::instance().critical(__VA_ARGS__)

#elif defined(COTS_RELEASE)

#define LOG_TRACE(...) ((void)0)
#define LOG_DEBUG(...) ((void)0)
#define LOG_INFO(...) ((void)0)
#define LOG_WARN(...) ::cots::utils::logger::instance().warn(__VA_ARGS__)
#define LOG_ERROR(...) ::cots::utils::logger::instance().error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::cots::utils::logger::instance().critical(__VA_ARGS__)

#else //~ Production

#define LOG_TRACE(...) ((void)0)
#define LOG_DEBUG(...) ((void)0)
#define LOG_INFO(...) ((void)0)
#define LOG_WARN(...) ((void)0)
#define LOG_ERROR(...) ((void)0)
#define LOG_CRITICAL(...) ((void)0)

#endif

#endif //CURSEOFTHESEA_LOGGER_H
