// Created by Niffoxic (Harsh Dubey)
#include "engine/engine.h"
#include <spdlog/spdlog.h>

#include "engine/core/cots_assert.h"
#include "engine/registry.h"

cots::engine::engine() {}

cots::engine::~engine() {}

bool cots::engine::init()
{
    timer_ = service_locator::resolve<timer>();
    return true;
}

int cots::engine::execute()
{
    timer_->reset();
    float delta_time = 0.f;
    int fps = 0;

    timer_->set_target_frame_ps(160);
    while (true)
    {
        timer_->step();
        ++fps;
        delta_time += timer_->delta_time();
        if (delta_time > 1.f)
        {
            delta_time = 0.f;
            spdlog::info("FPS: {}", fps);
            fps = 0;
        }
        if (timer_->elapsed_time() > 10.f) break;
    }
    return 0;
}
