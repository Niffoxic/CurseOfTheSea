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

#include "trishul/core/interface/slot_map.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace trishul
{
    // monotonic high resolution stopwatch and frame pacer backed by qpc
    class timer
    {
    public:
         timer() noexcept;
        ~timer() noexcept = default;

        timer(const timer&) noexcept = default;
        timer(timer&&)      noexcept = default;

        timer& operator=(const timer&) noexcept = default;
        timer& operator=(timer&&)      noexcept = default;

        void reset () noexcept;
        void pause () noexcept;
        void resume() noexcept;

        //~ advance one frame measure delta and pace to target when capped
        void step() noexcept;

        //~ frame cap zero disables pacing uncapped
        void set_target_fps     (std::uint32_t fps) noexcept;
        void set_target_frame_ms(float ms)          noexcept;
        void set_uncapped       ()                  noexcept;

        [[nodiscard]] float  delta_time      () const noexcept; // seconds
        [[nodiscard]] float  delta_time_ms   () const noexcept;
        [[nodiscard]] double elapsed_time    () const noexcept; // since reset
        [[nodiscard]] double elapsed_time_ms () const noexcept;
        [[nodiscard]] float  fps             () const noexcept; // from last delta
        [[nodiscard]] bool   paused          () const noexcept;

        [[nodiscard]] static std::string current_date() noexcept;

        //~ raw cpu timestamp counters for micro profiling
        [[nodiscard]] static std::uint64_t cpu_cycles       () noexcept;
        [[nodiscard]] static std::uint64_t cpu_cycles_serial() noexcept;

    private:
        std::int64_t start_ticks_    { 0 };
        std::int64_t previous_ticks_ { 0 };
        std::int64_t pause_ticks_    { 0 };
        std::int64_t delta_ticks_    { 0 };
        std::int64_t target_ticks_   { 0 }; // 0 uncapped
        std::int64_t next_deadline_  { 0 }; // absolute pacing grid
        bool         paused_         { false };
    };

    struct timer_tag {};
    using timer_handle = handle<timer_tag>;

    class timer_manager
    {
    public:
        using callback = std::function<void()>;

        //~ fire fn after delay seconds repeating every delay when looping
        timer_handle set_timer(callback fn, float delay, bool loop = false);
        //~ looping with a distinct first delay
        timer_handle set_timer(callback fn, float rate, bool loop, float first_delay);

        bool clear    (timer_handle h) noexcept;
        void clear_all() noexcept;

        [[nodiscard]] bool  is_active(timer_handle h) const noexcept;
        [[nodiscard]] float remaining(timer_handle h) const noexcept; // to next fire
        [[nodiscard]] float elapsed  (timer_handle h) const noexcept; // into cycle

        void pause (timer_handle h) noexcept;
        void resume(timer_handle h) noexcept;

        //~ advance every timer and fire the due ones
        void tick(float dt);

        [[nodiscard]] std::uint32_t active_count() const noexcept;

    private:
        struct entry
        {
            callback     fn;
            float        remaining { 0.f };
            float        rate      { 0.f };
            float        duration  { 0.f }; // current cycle length
            bool         loop      { false };
            bool         paused    { false };
            timer_handle self      {};
        };

        slot_map<entry, timer_tag> timers_;
        std::vector<timer_handle>  due_; // scratch reused each tick
    };
} // namespace trishul

#endif //CURSEOFTHESEA_TIMER_H
