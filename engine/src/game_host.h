// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_GAME_HOST_H
#define CURSEOFTHESEA_GAME_HOST_H

#include <cots/game_api.h>
#include <cots/game_memory.h>
#include <cots/engine_services.h>

#include <windows.h>
#include <cstdint>
#include <chrono>
#include <string>

namespace cots::game
{
    class host
    {
        using ms = std::chrono::milliseconds;
    public:
         host() = default;
        ~host();

        host(const host&) = delete;
        host(host&&)      = delete;

        host& operator=(const host&) = delete;
        host& operator=(host&&)      = delete;

        // allocate game memory and services and loads dll
        [[nodiscard]] bool initialize(const module::services& services);
                      void deinitialize(); // free mem and call notify game dll

        // checks if the game dll is changed
        bool pool_for_reload();
        void update(float dt);

    private:
        [[nodiscard]] bool load_dll  ();
                      void unload_dll();

    private:
        module::api      api_     {};
        module::memory   memory_  {};
        module::services services_{};

        struct load_record
        {
            std::uint16_t retry_attempts     { 10 };
            HMODULE       dll_handle_        { nullptr };
            FILETIME      watched_write_time_{};
            std::string   dll_source_path_   { "game.dll" };
            std::string   dll_live_path_     { "game_live.dll" };
            ms            sleep_time_        { 100 };
        } record_;
    };
}

#endif //CURSEOFTHESEA_GAME_HOST_H
