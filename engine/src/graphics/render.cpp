// Created by Niffoxic (Harsh Dubey)

#include "engine/graphics/render.h"
#include "engine/graphics/hardware/device.h"
#include "engine/graphics/hardware/fence.h"
#include "engine/graphics/hardware/swapchain.h"

#include "engine/core/cots_assert.h"
#include "engine/system/define_features.h"
#include "spdlog/spdlog.h"

#include "engine/events/event_dispatcher.h"
#include "engine/events/windows_event.h"

#include <d3d12.h>

#include "engine/graphics/hardware/command_context.h"

cots::graphics::render::~render() = default;

bool cots::graphics::render::initialize()
{
    subscribe_events();

    //~ initialize device
    if (not device_.initialize())
    {
        spdlog::error("device init failed");
        return false;
    }

    //~ initialize fence
    if (not fence_.initialize(device_))
    {
        spdlog::error("fence init failed");
        return false;
    }

    //~ initialize swapchain
    const auto windows = feature::locator::resolve<platform::windows>();
    const auto size = windows->get_window_size<std::uint32_t>();

    hardware::swapchain_create_info swapchain_info{};
    swapchain_info.allow_tearing = true;
    swapchain_info.width         = size.width;
    swapchain_info.height        = size.height;
    swapchain_info.mode          = hardware::display_mode::windowed;
    swapchain_info.frame_count   = 3;
    swapchain_info.window_handle = windows->get_window_handle();

    if (not swapchain_.initialize(device_, swapchain_info))
    {
        spdlog::error("swapchain init failed");
        return false;
    }

    //~ test command context
    hardware::command_context test_ctx;
    if (!test_ctx.initialize(device_))
    {
        spdlog::error("cmd context init failed");
        return false;
    }
    //~ reset/close cycle
    if (not test_ctx.reset())
    {
        spdlog::error("reset failed");
        return false;
    }
    spdlog::info("[cmd-test] reset ok, list open");

    if (not test_ctx.close())
    {
        spdlog::error("close failed");
        return false;
    }
    spdlog::info("[cmd-test] close ok");

    //~ must work without GPU sync since we didn't submit anything
    if (!test_ctx.reset())
    {
        spdlog::error("2nd reset failed"); return false;
    }
    if (!test_ctx.close())
    {
        spdlog::error("2nd close failed"); return false;
    }
    spdlog::info("[cmd-test] second cycle ok");

    test_ctx.deinitialize();

    running_ = true;
    render_thread_ = std::thread(&render::render_thread_main, this);
    return true;
}

void cots::graphics::render::deinitialize() noexcept
{
    if (!running_.exchange(false)) return;

    if (render_thread_.joinable())
        render_thread_.join();

    fence_    .deinitialize();
    swapchain_.deinitialize();
    device_   .deinitialize();

    unsubscribe_events();
    spdlog::info(("Renderer deinitialized"));
}

void cots::graphics::render::begin_update(float dt)
{

}

void cots::graphics::render::end_update()
{

}

cots::graphics::hardware::device& cots::graphics::render::device() noexcept
{
    return device_;
}

cots::graphics::hardware::swapchain& cots::graphics::render::swapchain() noexcept
{
    return swapchain_;
}

cots::graphics::hardware::fence& cots::graphics::render::fence() noexcept
{
    return fence_;
}

const cots::graphics::hardware::device & cots::graphics::render::device() const noexcept
{
    return device_;
}

const cots::graphics::hardware::swapchain & cots::graphics::render::swapchain() const noexcept
{
    return swapchain_;
}

const cots::graphics::hardware::fence & cots::graphics::render::fence() const noexcept
{
    return fence_;
}

void cots::graphics::render::render_thread_main()
{
    spdlog::info("render thread started");
    SetThreadDescription(GetCurrentThread(), L"Cots Renderer");

    while (running_.load(std::memory_order_acquire))
    {
        //~ drain commands
        {
            std::lock_guard lock(command_mutex_);
            if (has_pending_resize_)
            {
                const bool status = swapchain_.resize(
                    device_,
                    event_pending_resize_.width,
                    event_pending_resize_.height
                );
                if (not status) spdlog::error("swapchain resize failed");
                has_pending_resize_ = false;
            }
        }

        //~ occlusion check
        if (!swapchain_.check_occlusion())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        begin_frame();
        //~ TODO: Record Cmds
        end_frame();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    spdlog::info("render thread stopped");
}

void cots::graphics::render::record_frame()
{

}

void cots::graphics::render::submit_frame()
{

}

void cots::graphics::render::draw_frame()
{

}

void cots::graphics::render::subscribe_events()
{
    const auto dispatcher = feature::locator::resolve<events::dispatcher>();
    dispatcher->subscribe<events::window_resized,
    &render::on_window_resized>(*this);
}

void cots::graphics::render::unsubscribe_events()
{
    const auto dispatcher = feature::locator::resolve<events::dispatcher>();
    dispatcher->unsubscribe<events::window_resized,
    &render::on_window_resized>(*this);
}

void cots::graphics::render::on_window_resized(const events::window_resized &event)
{
    std::lock_guard lock(command_mutex_);
    event_pending_resize_ = event;
    has_pending_resize_   = true;
}
