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
#include "trishul/utils/timer.h"

#include <windows.h>
#include <intrin.h>
#include <immintrin.h>

#include <algorithm>
#include <cstdio>

#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif

namespace trishul
{
    namespace
    {
        std::int64_t query_frequency() noexcept
        {
            LARGE_INTEGER f;
            QueryPerformanceFrequency(&f);
            return f.QuadPart;
        }

        const std::int64_t g_qpc_freq     = query_frequency();
        const double       g_sec_per_tick = 1.0 / static_cast<double>(g_qpc_freq);

        __forceinline std::int64_t qpc_now() noexcept
        {
            LARGE_INTEGER c;
            QueryPerformanceCounter(&c);
            return c.QuadPart;
        }

        __forceinline std::int64_t ticks_from_seconds(const double s) noexcept
        {
            return static_cast<std::int64_t>(s * static_cast<double>(g_qpc_freq));
        }

        //~ one high resolution waitable timer per thread reused across waits
        HANDLE thread_waitable() noexcept
        {
            thread_local HANDLE h = CreateWaitableTimerExW(
                nullptr, nullptr,
                CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                TIMER_ALL_ACCESS);
            return h;
        }

        void wait_until(const std::int64_t deadline) noexcept
        {
            //~ spin the final slice to absorb sleep and timer overshoot
            constexpr double spin_slice_ms = 2.0;

            for (;;)
            {
                const std::int64_t now       = qpc_now();
                const std::int64_t remaining = deadline - now;
                if (remaining <= 0) return;

                const double rem_ms =
                    static_cast<double>(remaining) * 1000.0 * g_sec_per_tick;

                if (rem_ms > spin_slice_ms)
                {
                    const double sleep_ms = rem_ms - spin_slice_ms;

                    if (HANDLE h = thread_waitable())
                    {
                        LARGE_INTEGER due;
                        //~ negative is a relative
                        due.QuadPart = -static_cast<LONGLONG>(sleep_ms * 10'000.0);

                        if (SetWaitableTimerEx(h, &due, 0, nullptr, nullptr, nullptr, 0))
                        {
                            WaitForSingleObject(h, INFINITE);
                            continue;
                        }
                    }

                    //~ fallback coarse sleep
                    Sleep(static_cast<DWORD>(sleep_ms)); //~ bad tho
                }
                else
                {
                    //~ fine grained busy wait tell the core we are spinning
                    _mm_pause();
                    _mm_pause();
                    _mm_pause();
                    _mm_pause();
                    _mm_pause();
                    _mm_pause();
                    _mm_pause();
                    _mm_pause();
                }
            }
        }
    } // namespace anonymous

    timer::timer() noexcept
    {
        reset();
    }

    void timer::reset() noexcept
    {
        start_ticks_    = qpc_now();
        previous_ticks_ = start_ticks_;
        pause_ticks_    = 0;
        delta_ticks_    = 0;
        next_deadline_  = 0; // re anchor on next step
        paused_         = false;
    }

    void timer::step() noexcept
    {
        if (paused_)
        {
            delta_ticks_ = 0;
            return;
        }

        if (target_ticks_ > 0)
        {
            //~ pace on an absolute grid so isolated overshoots self correct
            if (next_deadline_ == 0)
                next_deadline_ = previous_ticks_ + target_ticks_;

            wait_until(next_deadline_);

            const std::int64_t now = qpc_now();
            delta_ticks_    = now - previous_ticks_;
            previous_ticks_ = now;

            //~ advance the grid drop any fully missed frames so a single
            //  hitch stays a single hitch with no catch up burst
            next_deadline_ += target_ticks_;
            while (next_deadline_ <= now)
                next_deadline_ += target_ticks_;
            return;
        }

        const std::int64_t now = qpc_now();
        delta_ticks_    = now - previous_ticks_;
        previous_ticks_ = now;
    }

    void timer::pause() noexcept
    {
        if (paused_) return;
        paused_      = true;
        pause_ticks_ = qpc_now();
        delta_ticks_ = 0;
    }

    void timer::resume() noexcept
    {
        if (!paused_) return;
        paused_ = false;

        //~ do not count the paused span shift the baselines forward
        const std::int64_t span = qpc_now() - pause_ticks_;
        previous_ticks_ += span;
        start_ticks_    += span;
        next_deadline_   = 0; // re anchor the grid
    }

    void timer::set_target_fps(std::uint32_t fps) noexcept
    {
        next_deadline_ = 0;
        if (fps == 0u) { target_ticks_ = 0; return; }
        fps = std::clamp<std::uint32_t>(fps, 1u, 1000u);
        target_ticks_ = g_qpc_freq / static_cast<std::int64_t>(fps);
    }

    void timer::set_target_frame_ms(const float ms) noexcept
    {
        next_deadline_ = 0;
        if (ms <= 0.f) { target_ticks_ = 0; return; }
        target_ticks_ = ticks_from_seconds(static_cast<double>(ms) / 1000.0);
    }

    void timer::set_uncapped() noexcept
    {
        target_ticks_  = 0;
        next_deadline_ = 0;
    }

    float timer::delta_time() const noexcept
    {
        return static_cast<float>(static_cast<double>(delta_ticks_) * g_sec_per_tick);
    }

    float timer::delta_time_ms() const noexcept
    {
        return static_cast<float>(static_cast<double>(delta_ticks_) * g_sec_per_tick * 1000.0);
    }

    double timer::elapsed_time() const noexcept
    {
        const std::int64_t now = paused_ ? pause_ticks_ : qpc_now();
        return static_cast<double>(now - start_ticks_) * g_sec_per_tick;
    }

    double timer::elapsed_time_ms() const noexcept
    {
        return elapsed_time() * 1000.0;
    }

    float timer::fps() const noexcept
    {
        if (delta_ticks_ <= 0) return 0.f;
        return static_cast<float>(
            static_cast<double>(g_qpc_freq) / static_cast<double>(delta_ticks_));
    }

    bool timer::paused() const noexcept
    {
        return paused_;
    }

    std::string timer::current_date() noexcept
    {
        SYSTEMTIME st;
        GetLocalTime(&st);

        char buf[32];
        std::snprintf(buf, sizeof(buf), "%04u-%02u-%02u %02u:%02u:%02u",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        return buf;
    }

    std::uint64_t timer::cpu_cycles() noexcept
    {
        return __rdtsc();
    }

    std::uint64_t timer::cpu_cycles_serial() noexcept
    {
        //~ fence so surrounding loads do not cross the read
        _mm_lfence();
        unsigned aux;
        const std::uint64_t tsc = __rdtscp(&aux);
        _mm_lfence();
        return tsc;
    }

    timer_handle timer_manager::set_timer(callback fn, const float delay, const bool loop)
    {
        return set_timer(std::move(fn), delay, loop, delay);
    }

    timer_handle timer_manager::set_timer(
        callback fn, const float rate, const bool loop, const float first_delay)
    {
        entry e;
        e.fn        = std::move(fn);
        e.remaining = first_delay;
        e.rate      = rate;
        e.duration  = first_delay;
        e.loop      = loop;
        e.paused    = false;

        const timer_handle h = timers_.emplace(std::move(e));
        if (entry* p = timers_.get(h)) p->self = h;
        return h;
    }

    bool timer_manager::clear(const timer_handle h) noexcept
    {
        return timers_.erase(h);
    }

    void timer_manager::clear_all() noexcept
    {
        timers_.clear();
    }

    bool timer_manager::is_active(const timer_handle h) const noexcept
    {
        return timers_.contains(h);
    }

    float timer_manager::remaining(const timer_handle h) const noexcept
    {
        const entry* e = timers_.get(h);
        return e ? e->remaining : 0.f;
    }

    float timer_manager::elapsed(const timer_handle h) const noexcept
    {
        const entry* e = timers_.get(h);
        if (!e) return 0.f;
        const float into = e->duration - e->remaining;
        return into > 0.f ? into : 0.f;
    }

    void timer_manager::pause(const timer_handle h) noexcept
    {
        if (entry* e = timers_.get(h)) e->paused = true;
    }

    void timer_manager::resume(const timer_handle h) noexcept
    {
        if (entry* e = timers_.get(h)) e->paused = false;
    }

    void timer_manager::tick(float dt)
    {
        if (dt < 0.f) dt = 0.f;
        due_.clear();

        timers_.for_each([&](entry& e)
        {
            if (e.paused) return;
            e.remaining -= dt;
            if (e.remaining <= 0.f) due_.push_back(e.self);
        });

        for (const timer_handle h : due_)
        {
            entry* e = timers_.get(h);
            if (!e || e->paused) continue;

            callback    fn   = e->fn;
            const bool  loop = e->loop;
            const float rate = e->rate;

            if (loop)
            {
                e->remaining += rate;
                if (e->remaining <= 0.f) e->remaining = rate; // huge dt guard
                e->duration   = rate;
            }

            if (fn) fn();

            if (!loop) timers_.erase(h);
        }
    }

    std::uint32_t timer_manager::active_count() const noexcept
    {
        return timers_.size();
    }
} // namespace trishul
