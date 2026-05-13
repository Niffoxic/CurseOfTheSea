// Created by Niffoxic (Harsh Dubey)

#include "engine/graphics/render.h"
#include "engine/graphics/hardware/device.h"

#include "engine/core/cots_assert.h"
#include "engine/graphics/hardware/fence.h"
#include "spdlog/spdlog.h"

#include <d3d12.h>

cots::graphics::render::~render() = default;

bool cots::graphics::render::initialize()
{
    //~ test device
    hardware::device test_device;
    if (!test_device.initialize())
    {
        spdlog::error("device init failed");
        return false;
    }
    spdlog::info("{} adapters available",
                 test_device.adapters_info().size());

    //~ test fence
    hardware::fence test_fence;
    if (!test_fence.initialize(test_device))
    {
        spdlog::error("fence init failed");
        return false;
    }

    if (!test_fence.wait(0))
    {
        spdlog::error("fence wait failed");
        return false;
    }
    spdlog::info("fence signaled");

    //~ test signal advances
    const auto target = test_fence.signal(test_device.graphics_queue());
    spdlog::info("fence singaled to: {}", target);

    if (!test_fence.wait(target))
    {
        spdlog::error("fence wait failed");
        return false;
    }

    spdlog::info("wait(target) returned, completed_value={}",
             test_fence.completed_value());

    //~ timeout on a value that will never be reached
    const bool reached = test_fence.wait(target + 100, 50);
    spdlog::info("wait unreachable 50ms returned {}", reached);

    test_fence.deinitialize();
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
