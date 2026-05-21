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
#include "engine/utils/helpers.h"

#include <d3d12.h>
#include <dxgi1_6.h>

#include "engine/graphics/hardware/command_context.h"
#include "engine/utils/profiler.h"

cots::graphics::render::~render() = default;

bool cots::graphics::render::initialize()
{
    running_ = true;
    render_thread_ = std::thread(&render::render_thread_main, this);
    return true;
}

void cots::graphics::render::deinitialize() noexcept
{
    if (!running_.exchange(false)) return;

    if (render_thread_.joinable())
        render_thread_.join();

    spdlog::info(("Renderer deinitialized"));
}

void cots::graphics::render::begin_update(float dt)
{

}

void cots::graphics::render::end_update()
{

}

cots::graphics::hardware::swapchain& cots::graphics::render::swapchain() noexcept
{
    return swapchain_;
}

const cots::graphics::hardware::swapchain & cots::graphics::render::swapchain() const noexcept
{
    return swapchain_;
}

void cots::graphics::render::render_thread_main()
{
    if (not initialize_render_thread())
    {
        running_ = false;
        spdlog::error("render thread init failed");
        return;
    }

    spdlog::info("render thread started");
    SetThreadDescription(GetCurrentThread(), L"Cots Renderer");
    frame_.start_time_ = std::chrono::steady_clock::now();

    while (running_.load(std::memory_order_relaxed))
    {
        process_pending_commands();
        //~ occlusion check
        if (!swapchain_.check_occlusion())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        draw_frame();
    }
    if (not fence_.wait(fence_.last_signaled_value())) [[unlikely]]
    {
        spdlog::error("[very unexpected] fence wait failed");
    }

    fence_    .deinitialize();
    swapchain_.deinitialize();
    device_   .deinitialize();

    unsubscribe_events();
    spdlog::info("render thread stopped");
}

bool cots::graphics::render::initialize_render_thread()
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
    //~ initialize contexts
    for (std::uint32_t i = 0; i < hardware::frame_count; ++i)
    {
        if (not frame_.contexts[i].initialize(device_, hardware::command_list_type::direct))
        {
            spdlog::error("command list init failed");
            return false;
        }
    }
    frame_.fence_values.fill(0);
    frame_.index = 0u;
    frame_.submit_lists.reserve(hardware::max_submit_lists);
    return true;
}

void cots::graphics::render::draw_frame()
{
    COTS_PROFILE_SCOPE("render::draw_frame");

    const std::uint32_t frame = frame_.index;

    {
        COTS_PROFILE_BEGIN("wait frame fence", helpers::markers::frame);
        fence_.wait(frame_.fence_values[frame]);
        COTS_PROFILE_END();
    }

    frame_.submit_lists.clear();
    record_frame(frame, frame_.submit_lists);

    {
        COTS_PROFILE_BEGIN("submit", helpers::markers::submit);
        submit_frame(frame_.submit_lists);
        COTS_PROFILE_END();
    }

    {
        COTS_PROFILE_BEGIN("present", helpers::markers::present);
        swapchain_.present(0);
        COTS_PROFILE_END();
    }

    frame_.fence_values[frame] = fence_.signal(device_.graphics_queue());
    frame_.step();
}

void cots::graphics::render::record_frame(
    const std::uint32_t frame,
    std::vector<ID3D12CommandList*>& out)
{
    COTS_PROFILE_SCOPE("render::record_frame");

    auto& ctx = frame_.contexts[frame];
    if (!ctx.reset()) return;

    auto*      backbuffer = swapchain_.current_backbuffer();
    const auto rtv        = swapchain_.current_rtv_handle();

    COTS_GPU_PROFILE_BEGIN(ctx.list(), "clear backbuffer", helpers::markers::record);

    ctx.transition(backbuffer,
                   hardware::resource_state::present,
                   hardware::resource_state::render_target);

    ctx.set_render_target(rtv);

    const float t = std::chrono::duration<float>(
        std::chrono::steady_clock::now() - frame_.start_time_).count();
    const float color[4] =
    {
        0.5f + 0.5f * std::sin(t * 0.7f),
        0.5f + 0.5f * std::sin(t * 0.9f + 2.0f),
        0.5f + 0.5f * std::sin(t * 1.3f + 4.0f),
        1.0f
    };
    ctx.clear_render_target(rtv, color);

    ctx.transition(backbuffer,
                   hardware::resource_state::render_target,
                   hardware::resource_state::present);

    COTS_GPU_PROFILE_END(ctx.list());

    if (not ctx.close()) [[unlikely]]
    {
        spdlog::error("command list close failed");
        return;
    }
    out.push_back(ctx.list());
}

void cots::graphics::render::process_pending_commands()
{
    decltype(pending_) cmd;
    {
        std::lock_guard lock(command_mutex_);
        if (!pending_.any()) return;
        cmd      = pending_;
        pending_ = {};
    }

    COTS_PROFILE_SCOPE("render::swapchain_command");

    const std::uint64_t flush = fence_.signal(device_.graphics_queue());
    if (not fence_.wait(flush))
        spdlog::error("[render] gpu flush before swapchain change failed");

    bool ok = true;
    if (cmd.change_mode)  ok = swapchain_.set_display_mode (device_, cmd.mode) && ok;
    if (cmd.set_win_size) ok = swapchain_.set_windowed_size(device_, cmd.win_w, cmd.win_h) && ok;
    if (cmd.resize)       ok = swapchain_.resize           (device_, cmd.resize_w, cmd.resize_h) && ok;
    if (not ok) spdlog::error("[render] swapchain command(s) failed");

    frame_.fence_values.fill(flush);
    frame_.index = 0u;
}

void cots::graphics::render::submit_frame(const std::vector<ID3D12CommandList*> &lists) const
{
    //~ serial submission
    if (lists.empty()) return;

    device_.graphics_queue()->ExecuteCommandLists(
        static_cast<UINT>(lists.size()),
        lists.data()
    );
}


void cots::graphics::render::subscribe_events()
{
    const auto d = feature::locator::resolve<events::dispatcher>();
    d->subscribe<events::window_resized,                       &render::on_window_resized>(*this);
    d->subscribe<events::swapchain::set_display_mode,  &render::on_set_display_mode>(*this);
    d->subscribe<events::swapchain::set_windowed_size, &render::on_set_windowed_size>(*this);
}

void cots::graphics::render::unsubscribe_events()
{
    const auto d = feature::locator::resolve<events::dispatcher>();
    d->unsubscribe<events::window_resized,                       &render::on_window_resized>(*this);
    d->unsubscribe<events::swapchain::set_display_mode,  &render::on_set_display_mode>(*this);
    d->unsubscribe<events::swapchain::set_windowed_size, &render::on_set_windowed_size>(*this);
}

void cots::graphics::render::on_window_resized(const events::window_resized &event)
{
    std::lock_guard lock(command_mutex_);
    pending_.resize   = true;
    pending_.resize_w = event.width;
    pending_.resize_h = event.height;
}

void cots::graphics::render::on_set_display_mode(const events::swapchain::set_display_mode& event)
{
    std::lock_guard lock(command_mutex_);
    pending_.change_mode = true;
    pending_.mode        = event.mode;
}

void cots::graphics::render::on_set_windowed_size(const events::swapchain::set_windowed_size& event)
{
    std::lock_guard lock(command_mutex_);
    pending_.set_win_size = true;
    pending_.win_w = event.width;
    pending_.win_h = event.height;
}
