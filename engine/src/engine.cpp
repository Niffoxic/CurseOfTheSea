// Created by Niffoxic (Harsh Dubey)
#include "engine/engine.h"
#include <spdlog/spdlog.h>

#include "engine/core/cots_assert.h"
#include "engine/registry.h"
#include "engine/platform/platform_windows.h"

cots::engine::engine() {}

cots::engine::~engine() {}

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

    return true;
}

int cots::engine::execute()
{
    timer_->reset();

    timer_->set_target_frame_ps(160);
    while (true)
    {
        timer_->step();
        windows_->begin_frame(timer_->delta_time());
        if (windows_->should_close())
        {
            return 0;
        }
    }
    return 0;
}
