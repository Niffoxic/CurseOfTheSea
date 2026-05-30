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
#ifndef CURSEOFTHESEA_ENGINE_H
#define CURSEOFTHESEA_ENGINE_H

#include <cstdint>
#include <memory>
#include <string>

namespace trishul
{
    struct fps_information
    {
        //~ fps
        std::uint32_t main_thread   {};
        std::uint32_t render_thread {};
        std::uint32_t physics_thread{};

        //~ ms
        float main_thread_ms   {};
        float render_thread_ms {};
        float physics_thread_ms{};
    };

    //~ supplied by the game when it creates the engine
    struct engine_create_info
    {
        std::wstring  window_title    { L"Trishul Engine" };
        std::uint32_t window_width    { 1280 };
        std::uint32_t window_height   { 720 };

        int          icon_resource_id { 0 };
        std::wstring icon_path        {};

        //~ frame cap zero means uncapped
        std::uint32_t target_fps      { 0 };
    };

    class engine final
    {
    public:
        explicit engine(engine_create_info info = {});
        ~engine();

        //~ copy restricted
        engine           (const engine&) = delete;
        engine& operator=(const engine&) = delete;

        //~ move restricted
        engine           (engine&&) = delete;
        engine& operator=(engine&&) = delete;

        //~ life cycle
        [[nodiscard]] bool initialize() const;
                      void tick      () const;

        [[nodiscard]] bool            should_close() const noexcept;
        [[nodiscard]] float           delta_time  () const noexcept;
        [[nodiscard]] fps_information get_fps     () const noexcept;

    private:
        struct impl;
        std::unique_ptr<impl> p_;
    };
} // namespace trishul

#endif //CURSEOFTHESEA_ENGINE_H
