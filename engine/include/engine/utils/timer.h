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
#ifndef CURSEOFTHESEA_TIMER_H
#define CURSEOFTHESEA_TIMER_H

#include <chrono>

namespace cots::utils
{
    class timer
    {
        using clock = std::chrono::steady_clock;
        using Time  = std::chrono::steady_clock::time_point;
        using ms    = std::chrono::milliseconds;
        using ns    = std::chrono::nanoseconds;
    public:
         timer() noexcept = default;
        ~timer() noexcept = default;

        timer(const timer&) noexcept = default;
        timer(timer&&)      noexcept = default;

        timer& operator=(const timer&) noexcept = default;
        timer& operator=(timer&&)      noexcept = default;

        void reset () noexcept;
        void pause () noexcept;
        void resume() noexcept;
        void step  () noexcept;

        void set_target_frame_ps(int   target_frame_rate) noexcept;
        void set_target_frame_ms(float target_frame_rate) noexcept;

        [[nodiscard]] float delta_time     () const noexcept;
        [[nodiscard]] float delta_time_ms  () const noexcept;
        [[nodiscard]] float elapsed_time   () const noexcept;
        [[nodiscard]] float elapsed_time_ms() const noexcept;
        [[nodiscard]] bool  paused         () const noexcept;

        [[nodiscard]] static std::string current_date() noexcept;

        //~ helpers
        [[nodiscard]]
        static Time now() noexcept
        {
            return clock::now();
        }

        [[nodiscard]]
        static float elapsed_from(const Time& start)
        {
            const auto _now = now();
            return std::chrono::duration<float>(_now - start).count();
        }

    private:
        static constexpr float default_fps = 60.f;
        Time start_time_   {};
        Time previous_time_{};
        ns   delta_time_   {};

        bool paused_    { false };
        ns target_frame_{ ns(static_cast<long>(1'000'000'000.0f / default_fps))};
    };
} // namespace cots::utils

#endif //CURSEOFTHESEA_TIMER_H
