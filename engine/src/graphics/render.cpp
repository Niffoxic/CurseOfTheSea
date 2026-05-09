// Created by Niffoxic (Harsh Dubey)

#include "engine/graphics/render.h"
#include "engine/graphics/hardware/device.h"

#include "engine/core/cots_assert.h"
#include "spdlog/spdlog.h"

cots::graphics::render::~render() = default;

bool cots::graphics::render::initialize()
{
    hardware::device test_device;
    if (!test_device.initialize())
    {
        spdlog::error("[render] device init failed");
        return false;
    }

    spdlog::info("[render] {} adapters available",
                 test_device.adapters_info().size());

    test_device.deinitialize();

    running_ = true;
    render_thread_ = std::thread(&render::render_thread_main, this);
    return true;
}

void cots::graphics::render::deinitialize() noexcept
{
    if (!running_.exchange(false)) return;

    if (render_thread_.joinable()) render_thread_.join();
    spdlog::info(("Renderer deinitialized"));
}

void cots::graphics::render::begin_update(float dt)
{

}

void cots::graphics::render::end_update()
{

}

void cots::graphics::render::render_thread_main()
{
    spdlog::info("render thread started");
    SetThreadDescription(GetCurrentThread(), L"Cots Renderer");

    while (running_.load(std::memory_order_acquire))
    {
        begin_frame();
        //~ TODO: Record Cmds
        end_frame();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    spdlog::info("render thread stopped");
}

void cots::graphics::render::begin_frame()
{

}

void cots::graphics::render::end_frame()
{

}
