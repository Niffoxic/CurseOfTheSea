// Created by Niffoxic (Harsh Dubey)
#include "engine/engine.h"

#include <iostream>
#include <spdlog/spdlog.h>

#include "engine/service_locator.h"
#include "engine/core/cots_assert.h"

cots::engine::engine() {}

cots::engine::~engine() {}

struct ILogger { virtual void log(const std::string&) = 0; virtual ~ILogger() = default; };
struct ConsoleLogger : ILogger { void log(const std::string& m) override { std::cout << m << "\n"; } };
struct NullLogger    : ILogger { void log(const std::string&) override {} };

bool cots::engine::init()
{
    service_locator::provide_null<ILogger>(std::make_shared<NullLogger>());
    service_locator::provide<ILogger>(std::make_shared<ConsoleLogger>());
    std::string test = "hello world";
    service_locator::resolve<ILogger>()->log(test);
    return true;
}

int cots::engine::execute()
{
    timer_.reset();
    float delta_time = 0.f;
    int fps = 0;

    timer_.set_target_frame_ps(160);
    while (true)
    {
        timer_.step();
        ++fps;
        delta_time += timer_.delta_time();
        if (delta_time > 1.f)
        {
            delta_time = 0.f;
            spdlog::info("FPS: {}", fps);
            fps = 0;
        }
        if (timer_.elapsed_time() > 10.f) break;
    }
    return 0;
}
