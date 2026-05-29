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
#ifndef CURSEOFTHESEA_LOGGER_H
#define CURSEOFTHESEA_LOGGER_H

#include <windows.h>

#include "trishul/core/interface/singleton.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <chrono>
#include <cstdio>
#include <format>
#include <ios>
#include <memory>
#include <string>
#include <vector>

namespace trishul
{
    class logger final: public interfaces::singleton<logger>
    {
        ENGINE_SINGLETON(logger)
         logger() = default;
        ~logger() = default;
    public:
        void initialize()
        {
            if (initialized_) return;

            std::vector<spdlog::sink_ptr> sinks;

#if defined(COTS_DEBUG) || defined(COTS_RELWITHDEBINFO)
            if (GetConsoleWindow() == nullptr && AllocConsole())
            {
                FILE* dummy = nullptr;
                freopen_s(&dummy, "CONOUT$", "w", stdout);
                freopen_s(&dummy, "CONOUT$", "w", stderr);
                freopen_s(&dummy, "CONIN$",  "r", stdin);
                std::ios::sync_with_stdio(true);
                SetConsoleTitleA("Curse of the Sea Debug Console");
                owns_console_ = true;
            }

            auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console->set_pattern("%^[%H:%M:%S.%e] [%-8l] %v%$");
            sinks.push_back(std::move(console));
#endif
            //~ persistent trail one file per run spdlog makes the dir
            try
            {
                const auto stamp = std::chrono::current_zone()->to_local(
                    std::chrono::floor<std::chrono::seconds>(
                        std::chrono::system_clock::now()));
                const std::string path =
                    std::format(".logs/log-{:%Y-%m-%d_%H-%M-%S}.txt", stamp);

                auto file = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path, true);
                file->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%-8l] [t %t] %v");
                sinks.push_back(std::move(file));
            }
            catch (...)
            {
                //~ file unavailable
            }

            auto core = std::make_shared<spdlog::logger>(
                "trishul", sinks.begin(), sinks.end());

            core->flush_on(spdlog::level::warn);

#if defined(COTS_DEBUG)
            core->set_level(spdlog::level::trace);
#elif defined(COTS_RELWITHDEBINFO)
            core->set_level(spdlog::level::debug);
#elif defined(COTS_RELEASE)
            core->set_level(spdlog::level::warn);
#else
            core->set_level(spdlog::level::off);
#endif
            spdlog::set_default_logger(std::move(core));
            initialized_ = true;

            spdlog::info("logger online");
        }

        void deinitialize()
        {
            if (!initialized_) return;

            spdlog::shutdown();
            initialized_ = false;

#if defined(COTS_DEBUG) || defined(COTS_RELWITHDEBINFO)
            if (owns_console_ && GetConsoleWindow() != nullptr)
            {
                FreeConsole();
                owns_console_ = false;
            }
#endif
        }

        [[nodiscard]] bool is_initialized() const noexcept { return initialized_; }

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
        void critical(spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            spdlog::critical(fmt, std::forward<Args>(args)...);
        }

        void set_level(const spdlog::level::level_enum level)
        {
            spdlog::set_level(level);
        }

    private:
        bool initialized_  { false };
        bool owns_console_ { false };
    };
} // namespace trishul

#if defined(COTS_DEBUG) //~ log everything

#define LOG_TRACE(...)    ::trishul::logger::instance().trace(__VA_ARGS__)
#define LOG_DEBUG(...)    ::trishul::logger::instance().debug(__VA_ARGS__)
#define LOG_INFO(...)     ::trishul::logger::instance().info(__VA_ARGS__)
#define LOG_WARN(...)     ::trishul::logger::instance().warn(__VA_ARGS__)
#define LOG_ERROR(...)    ::trishul::logger::instance().error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::trishul::logger::instance().critical(__VA_ARGS__)

#elif defined(COTS_RELWITHDEBINFO) //~ keeps useful logs

#define LOG_TRACE(...)    ((void)0)
#define LOG_DEBUG(...)    ::trishul::logger::instance().debug(__VA_ARGS__)
#define LOG_INFO(...)     ::trishul::logger::instance().info(__VA_ARGS__)
#define LOG_WARN(...)     ::trishul::logger::instance().warn(__VA_ARGS__)
#define LOG_ERROR(...)    ::trishul::logger::instance().error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::trishul::logger::instance().critical(__VA_ARGS__)

#elif defined(COTS_RELEASE)

#define LOG_TRACE(...)    ((void)0)
#define LOG_DEBUG(...)    ((void)0)
#define LOG_INFO(...)     ((void)0)
#define LOG_WARN(...)     ::trishul::logger::instance().warn(__VA_ARGS__)
#define LOG_ERROR(...)    ::trishul::logger::instance().error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::trishul::logger::instance().critical(__VA_ARGS__)

#else //~ production silence

#define LOG_TRACE(...)    ((void)0)
#define LOG_DEBUG(...)    ((void)0)
#define LOG_INFO(...)     ((void)0)
#define LOG_WARN(...)     ((void)0)
#define LOG_ERROR(...)    ((void)0)
#define LOG_CRITICAL(...) ((void)0)

#endif

#endif //CURSEOFTHESEA_LOGGER_H
