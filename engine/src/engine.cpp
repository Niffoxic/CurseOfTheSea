// Created by Niffoxic (Harsh Dubey)
#include "engine/engine.h"
#include <spdlog/spdlog.h>

#include "engine/core/cots_assert.h"
#include "engine/registry.h"
#include "engine/platform/platform_windows.h"

cots::engine::engine() = default;

cots::engine::~engine() = default;

bool cots::engine::init()
{
    timer_   = service_locator::resolve<utils::timer>();
    windows_ = service_locator::resolve<platform::windows>();

    platform::initialize_info window_info{};
    window_info.window_size = {1280, 720};
    window_info.window_title = L"Curse of the sea";

    COTS_ASSERT_MSG(windows_ != nullptr, "Failed to initialize windows");
    if (not windows_->initialize(window_info))
    {
        return false;
    }

    timer_->reset();
    timer_->set_target_frame_ps(160);

    return true;
}

void cots::engine::tick()
{
    timer_->step();
    windows_->begin_frame(timer_->delta_time());
}

bool cots::engine::should_close() const noexcept
{
    return windows_->should_close();
}

float cots::engine::delta_time() const noexcept
{
    return timer_->delta_time();
}
