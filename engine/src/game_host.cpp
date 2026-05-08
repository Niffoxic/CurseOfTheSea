// Created by Niffoxic (Harsh Dubey)
#include "game_host.h"
#include "engine/core/cots_assert.h"
#include <spdlog/spdlog.h>

namespace
{
    FILETIME get_file_write_time(const char* path)
    {
        FILETIME ft{};
        WIN32_FILE_ATTRIBUTE_DATA data;

        if (GetFileAttributesExA(path, GetFileExInfoStandard, &data))
            ft = data.ftLastWriteTime;

        return ft;
    }
}

namespace cots::game
{
    host::~host()
    {
        deinitialize();
    }

    bool host::initialize(const module::services& services)
    {
        services_ = services;

        //~ 64 mb blocks
        memory_.permanent_size = 64ull * 1024 * 1024;
        memory_.transient_size = 64ull * 1024 * 1024;

        memory_.permanent = VirtualAlloc(
            nullptr, memory_.permanent_size,
            MEM_RESERVE | MEM_COMMIT,
            PAGE_READWRITE
        );

        memory_.transient = VirtualAlloc(
            nullptr, memory_.permanent_size,
            MEM_RESERVE | MEM_COMMIT,
            PAGE_READWRITE
        );

        memory_.initialized = false;

        COTS_ASSERT_MSG(memory_.permanent != nullptr, "game::host: VirtualAlloc failed");
        COTS_ASSERT_MSG(memory_.transient != nullptr, "game::host: VirtualAlloc failed");

        if (!load_dll())
        {
            spdlog::error("Failed to load game dll");
            return false;
        }

        if (api_.on_load)
        {
            api_.on_load(&memory_, &services_);
        }

        return true;
    }

    void host::deinitialize()
    {
        if (api_.on_unload && record_.dll_handle_)
        {
            api_.on_unload(&memory_);
        }
        unload_dll();

        if (memory_.permanent)
        {
            VirtualFree(memory_.permanent, 0, MEM_RELEASE);
            memory_.permanent = nullptr;
        }

        if (memory_.transient)
        {
            VirtualFree(memory_.transient, 0, MEM_RELEASE);
            memory_.transient = nullptr;
        }
    }

    bool host::pool_for_reload()
    {
        const auto now = get_file_write_time(record_.dll_source_path_.c_str());

        if (CompareFileTime(&now, &record_.watched_write_time_) == 0)
        {
            return false;
        }

        spdlog::info("game::host: Detected game dll change: State Reloading");

        if (api_.on_unload) api_.on_unload(&memory_);

        unload_dll();

        //~ linker safety
        std::this_thread::sleep_for(record_.sleep_time_);

        if (not load_dll())
        {
            spdlog::error("game::host: Failed to load game dll");
            return false;
        }
        if (api_.on_load) api_.on_load(&memory_, &services_);

        spdlog::info("game::host: State Reloaded");
        return true;
    }

    void host::update(const float dt)
    {
        if (api_.update) api_.update(&memory_, dt);
    }

    bool host::load_dll()
    {
        for (size_t attempt = 0; attempt < record_.retry_attempts; ++attempt)
        {
            if (CopyFileA(
                record_.dll_source_path_.c_str(),
                record_.dll_live_path_.c_str(),
                FALSE))
            {
                break;
            }
            std::this_thread::sleep_for(record_.sleep_time_);
            if (attempt == record_.retry_attempts - 1)
            {
                spdlog::error("game::host: could not copy {} to {}",
                    record_.dll_source_path_,
                    record_.dll_live_path_);
                return false;
            }
        }

        record_.dll_handle_ = LoadLibraryA(record_.dll_live_path_.c_str());
        if (not record_.dll_handle_)
        {
            spdlog::error("game::host: Failed to load game dll");
            return false;
        }

        using get_api_fn = void(*)(module::api*);
        auto get_api = reinterpret_cast<get_api_fn>(GetProcAddress(record_.dll_handle_, "cots_get_game_api"));

        if (not get_api)
        {
            spdlog::error("game::host: cots_get_game_api not found in {}", record_.dll_live_path_);
            FreeLibrary(record_.dll_handle_);
            record_.dll_handle_ = nullptr;
            return false;
        }

        get_api(&api_);
        record_.watched_write_time_ = get_file_write_time(record_.dll_source_path_.c_str());
        return true;
    }

    void host::unload_dll()
    {
        if (record_.dll_handle_)
        {
            FreeLibrary(record_.dll_handle_);
            record_.dll_handle_ = nullptr;
        }
        std::memset(&api_, 0, sizeof(api_));
    }
} // namespace cots::game
