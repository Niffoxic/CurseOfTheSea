// Created by Niffoxic (Harsh Dubey)
#include "engine/engine.h"
#include <spdlog/spdlog.h>

#include <ranges>

#include "engine/core/cots_assert.h"
#include "engine/system/define_features.h"
#include "engine/platform/platform_windows.h"

cots::engine::engine() = default;

cots::engine::~engine()
{
    for (const auto subsystem: std::views::reverse(subsystem_scheduler_))
    {
        if (subsystem) subsystem->deinitialize();
    }
}

bool cots::engine::init()
{
    initialize_features();
    regulate_subsystems();
    regulate_tickable  ();

    timer_->reset();
    timer_->set_target_frame_ps(160);

    return true;
}

void cots::engine::tick()
{
    timer_->step();
    update_tickable();
}

bool cots::engine::should_close() const noexcept
{
    return windows_->should_close();
}

float cots::engine::delta_time() const noexcept
{
    return timer_->delta_time();
}

void cots::engine::initialize_features()
{
    timer_   = feature::locator::resolve<utils::timer>();

    //~ setup windows
    windows_ = feature::locator::resolve<platform::windows>();
    windows_->setup_config(reinterpret_cast<const std::byte*>(&config_manager_.windows_config()));
}

void cots::engine::regulate_subsystems()
{
    subsystem_scheduler_.register_type(std::ref(windows_));

    for (const auto subsystem: subsystem_scheduler_)
    {
        if (not subsystem->initialize())
        {
            spdlog::error("Failed to initialize subsystem");
            COTS_FAIL_MSG("Failed to initialize subsystem");
        }
    }
    spdlog::info("All Subsystem initialized");
}

void cots::engine::regulate_tickable()
{
    tickable_scheduler_.register_type(std::ref(windows_));
}

void cots::engine::update_tickable()
{
    const float dt = timer_->delta_time();

    //~ update begin
    for (const auto tickable: tickable_scheduler_)
    {
        if (tickable)
        {
            tickable->begin_update(dt);
        }
    }

    //~ update end
    for (const auto iter : std::views::reverse(tickable_scheduler_))
    {
        if (iter)
        {
            iter->end_update();
        }
    }
}
