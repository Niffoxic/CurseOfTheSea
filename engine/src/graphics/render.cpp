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
#include <dxgi1_6.h>

#include "engine/graphics/hardware/command_context.h"

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
    frame_.submit_lists.reserve(hardware::flight_count);
    return true;
}

void cots::graphics::render::draw_frame()
{
    const std::uint32_t frame = frame_.index;
    fence_.wait(frame_.fence_values[frame]);

    //~ seam record and then serial submit
    frame_.submit_lists.clear();
    record_frame(frame, frame_.submit_lists);
    submit_frame(frame_.submit_lists);

    //~ present and fence this slot
    swapchain_.present(0);
    frame_.fence_values[frame] = fence_.signal(device_.graphics_queue());
    frame_.step();
}

void cots::graphics::render::record_frame(
    const std::uint32_t frame,
    std::vector<ID3D12CommandList *> &out
)
{
    auto& ctx = frame_.contexts[frame];
    if (!ctx.reset()) return;

    auto*      backbuffer = swapchain_.current_backbuffer();
    const auto rtv        = swapchain_.current_rtv_handle();

    ctx.transition(backbuffer,
                   hardware::resource_state::present,
                   hardware::resource_state::render_target);

    ctx.set_render_target(rtv);

    //~ animated clear for test
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

    if (not ctx.close()) [[unlikely]]
    {
        spdlog::error("command list close failed");
    }
    out.push_back(ctx.list());   //~ one list now, many later
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

void cots::graphics::render::process_pending_commands()
{
    //~ drain commands
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
