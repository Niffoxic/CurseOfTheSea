// Created by Niffoxic (Harsh Dubey)
#include "engine/utils/timer.h"

#include <algorithm>
#include <thread>

void cots::utils::timer::reset() noexcept
{
    start_time_     = clock::now();
    delta_time_     = ns(0);
    previous_time_  = start_time_;
    paused_         = false;
}

void cots::utils::timer::step() noexcept
{
    if (paused_)
    {
        delta_time_ = ns(0);
        return;
    }
    const auto end = clock::now();

    if (const auto dt = std::chrono::duration_cast<ns>(end - previous_time_);
        dt < target_frame_)
    {
        std::this_thread::sleep_for(target_frame_ - dt - ns(100));
    }
    const auto ndt = target_frame_;
    delta_time_    = ndt;
    previous_time_ = end;
}

void cots::utils::timer::pause() noexcept
{
    paused_ = true;
}

void cots::utils::timer::resume() noexcept
{
    if (not paused_) return;
    paused_         = false;
    previous_time_  = clock::now();
}

void cots::utils::timer::set_target_frame_ps(int target_frame_rate) noexcept
{
    target_frame_rate = std::clamp(target_frame_rate, 25, 240);
    const double fps = 1'000'000'000.0 / static_cast<double>(target_frame_rate);
    target_frame_    = ns(static_cast<long long>(fps));
}

void cots::utils::timer::set_target_frame_ms(const float target_frame_rate) noexcept
{
    const float val = std::clamp(target_frame_rate, 4.1f, 40.f);
    target_frame_ = ns(static_cast<long long>(val * 1'000'000.0f));
}

float cots::utils::timer::delta_time() const noexcept
{
    return (static_cast<float>(delta_time_.count())) / 1'000'000'000.0f;
}

float cots::utils::timer::delta_time_ms() const noexcept
{
    return (static_cast<float>(delta_time_.count())) / 1'000'000.0f;
}

float cots::utils::timer::elapsed_time() const noexcept
{
    const auto end = clock::now();
    const auto e   = std::chrono::duration_cast<ns>(end - start_time_);
    return static_cast<float>(e.count()) / 1'000'000'000.0f;
}

float cots::utils::timer::elapsed_time_ms() const noexcept
{
    const auto end = clock::now();
    const auto e   = std::chrono::duration_cast<ns>(end - start_time_);
    return static_cast<float>(e.count()) / 1'000'000.0f;
}

std::string cots::utils::timer::current_date() noexcept
{
    const auto now = std::chrono::system_clock::now();
    const auto t   = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_s(&tm, &t);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return buf;
}

bool cots::utils::timer::paused() const noexcept
{
    return paused_;
}
