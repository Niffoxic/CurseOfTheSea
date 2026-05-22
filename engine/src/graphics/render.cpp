// Created by Niffoxic (Harsh Dubey)

#include "engine/graphics/render.h"
#include "engine/graphics/hardware/device.h"
#include "engine/graphics/hardware/fence.h"
#include "engine/graphics/hardware/swapchain.h"
#include "engine/graphics/hardware/command_context.h"

#include "engine/utils/profiler.h"

#include "engine/core/cots_assert.h"
#include "engine/system/define_features.h"
#include "spdlog/spdlog.h"

#include "engine/events/event_dispatcher.h"
#include "engine/events/windows_event.h"
#include "engine/utils/helpers.h"

#include <d3d12.h>
#include <dxgi1_6.h>

//~ render graph passes
#include "engine/graphics/passes/pass_context.h"
#include "engine/graphics/passes/pass.h"
#include "engine/graphics/passes/clear_pass.h"
#include "engine/graphics/passes/present_pass.h"
#include "engine/graphics/passes/triangle_pass.h"

//~ test
#include "engine/graphics/shaders/storage/binary_storage.h"
#include "engine/graphics/shaders/storage/json_storage.h"
#include <cots/cots_config.h>

cots::graphics::render::~render() = default;

bool cots::graphics::render::initialize()
{
    subscribe_events();
    running_ = true;
    render_thread_ = std::thread(&render::render_thread_main, this);
    return true;
}

void cots::graphics::render::deinitialize() noexcept
{
    if (!running_.exchange(false)) return;

    if (render_thread_.joinable())
        render_thread_.join();

    unsubscribe_events();
    spdlog::info(("Renderer deinitialized"));
}

void cots::graphics::render::begin_update(const float dt)
{
    if (!render_ready_.load(std::memory_order_acquire)) return;

    //~ publish what the game built last frame into pending
    auto& building = snapshots_.next_build();
    building.frame_id   = snapshots_.frame_counter++;
    building.delta_time = dt;

    publish_snapshot();
}

void cots::graphics::render::end_update()
{
}

cots::graphics::scene_snapshot & cots::graphics::render::building_snapshot() noexcept
{
    return snapshots_.next_build();
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
        if (not swapchain_.check_occlusion())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        const bool had_new = acquire_snapshot();
        draw_frame(snapshots_.latest());
    }

    if (not fence_.wait(fence_.last_signaled_value())) [[unlikely]]
    {
        spdlog::error("[very unexpected] fence wait failed");
    }

    //~ destroy resources
    shader_cache_.deinitialize();
    buffers_     .deinitialize();
    fence_       .deinitialize();
    swapchain_   .deinitialize();
    device_      .deinitialize();

    spdlog::info("render thread stopped");
}

bool cots::graphics::render::initialize_render_thread()
{
    //~ initialize device
    if (not device_.initialize())
    {
        spdlog::error("device init failed");
        return false;
    }

    if (not buffers_.initialize(device_))
    {
        spdlog::error("buffer manager init failed");
        return false;
    }

    //~ test
    {
        const std::uint32_t test_data[4] =
        {
            0xDEADBEEF, 0x12345678, 0xCAFEBABE, 0x0BADF00D
        };

        hardware::buffer_create_info bi{};
        bi.size_bytes   = sizeof(test_data);
        bi.kind         = hardware::buffer_kind::generic;
        bi.initial_data = test_data;
        bi.debug_name   = "test_buffer";

        const auto h = buffers_.create(bi);
        spdlog::info("[buf-test] created handle {{idx={}, gen={}}}, gpu_addr=0x{:X}",
                     h.index, h.generation, buffers_.gpu_address(h));

        buffers_.destroy(h);
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

    //~ Test
    //~ json in debug packed binary otherwise
#if COTS_DEBUG
    auto storage = std::make_unique<shaders::json_shader_storage>("compiled/shader_cache.json");
#else
    auto storage = std::make_unique<shaders::binary_shader_storage>("compiled/shader_cache.bin");
#endif
    if (not shader_cache_.initialize(std::move(storage)))
    {
        spdlog::error("shader cache init failed");
        return false;
    }

    //~ test compile
    {
        const auto vs = shader_cache_.get_or_compile(
            "assets/shaders/triangle.hlsl",
            "VSMain",
            shaders::shader_stage::vertex
        );
        const auto ps = shader_cache_.get_or_compile(
            "assets/shaders/triangle.hlsl",
            "PSMain",
            shaders::shader_stage::pixel
        );
        spdlog::info("[shader-test] vs={} bytes, ps={} bytes", vs.size, ps.size);
    }

    if (not build_passes()) return false;

    render_ready_.store(true, std::memory_order_release);
    return true;
}

void cots::graphics::render::draw_frame(const scene_snapshot& snap)
{
    COTS_PROFILE_SCOPE("render::draw_frame");

    using clock = std::chrono::steady_clock;

    //~ RT-only accumulator - no sync needed, only this thread touches it
    static clock::time_point window_start = clock::now();
    static double            accum_ms     = 0.0;
    static std::uint32_t     accum_frames = 0;

    const auto frame_begin = clock::now();
    const std::uint32_t frame = frame_.index;

    fence_.wait(frame_.fence_values[frame]);

    frame_.submit_lists.clear();
    record_frame(frame, snap, frame_.submit_lists);
    submit_frame(frame_.submit_lists);

    swapchain_.present(0);

    frame_.fence_values[frame] = fence_.signal(device_.graphics_queue());
    frame_.step();

    //~ telemetry
    const auto   frame_end = clock::now();
    accum_ms += std::chrono::duration<double, std::milli>(frame_end - frame_begin).count();
    ++accum_frames;

    const double elapsed = std::chrono::duration<double>(frame_end - window_start).count();
    if (elapsed >= 1.0 && accum_frames > 0)
    {
        stat_fps_     .store(static_cast<float>(accum_frames / elapsed),       std::memory_order_relaxed);
        stat_frame_ms_.store(static_cast<float>(accum_ms / accum_frames),      std::memory_order_relaxed);

        window_start = frame_end;
        accum_ms     = 0.0;
        accum_frames = 0;
    }
}

void cots::graphics::render::record_frame(
    const std::uint32_t frame,
    const scene_snapshot& snap,
    std::vector<ID3D12CommandList*>& out)
{
    auto& ctx = frame_.contexts[frame];
    if (!ctx.reset()) return;

    const pass_context pc
    {
        .ctx         = ctx,
        .snap        = snap,
        .backbuffer  = swapchain_.current_backbuffer(),
        .rtv_handle  = swapchain_.current_rtv_handle(),
        .width       = swapchain_.width(),
        .height      = swapchain_.height(),
        .frame_index = frame,
    };

    for (const auto& p : passes_)
    {
        COTS_GPU_PROFILE_BEGIN(ctx.list(), p->name(), 0xFF40C040);
        p->execute(pc);
        COTS_GPU_PROFILE_END(ctx.list());
    }

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

    COTS_PROFILE_SCOPE("render::pending_commands");

    //~ shader ops are CPU-only no GPU flush needed
    if (cmd.shader_save)   shader_cache_.flush();
    if (cmd.shader_clear)  shader_cache_.clear();
    if (cmd.shader_reload) shader_cache_.recompile(cmd.shader_reload_key);

    //~ swapchain ops need the GPU idle first
    if (cmd.change_mode || cmd.set_win_size || cmd.resize)
    {
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
}

bool cots::graphics::render::build_passes()
{
    passes_.clear();
    passes_.push_back(std::make_unique<passes::clear_pass>());
    passes_.push_back(std::make_unique<passes::triangle_pass>());
    passes_.push_back(std::make_unique<passes::present_pass>());

    const setup_context sc
    {
        .device  = device_,
        .shaders = shader_cache_,
        .buffers = buffers_
    };

    for (auto& p : passes_)
    {
        if (not p->setup(sc))
        {
            spdlog::error("[render] pass '{}' setup failed", p->name());
            return false;
        }
    }
    spdlog::info("[render] {} pass(es) ready", passes_.size());
    return true;
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
    d->subscribe<events::window_resized,               &render::on_window_resized>   (*this);
    d->subscribe<events::swapchain::set_display_mode,  &render::on_set_display_mode> (*this);
    d->subscribe<events::swapchain::set_windowed_size, &render::on_set_windowed_size>(*this);
    d->subscribe<events::shader::save,                 &render::on_shader_save>      (*this);
    d->subscribe<events::shader::clear,                &render::on_shader_clear>     (*this);
    d->subscribe<events::shader::reload,               &render::on_shader_reload>    (*this);
}

void cots::graphics::render::unsubscribe_events()
{
    const auto d = feature::locator::resolve<events::dispatcher>();
    d->unsubscribe<events::window_resized,               &render::on_window_resized>   (*this);
    d->unsubscribe<events::swapchain::set_display_mode,  &render::on_set_display_mode> (*this);
    d->unsubscribe<events::swapchain::set_windowed_size, &render::on_set_windowed_size>(*this);
    d->unsubscribe<events::shader::save,                 &render::on_shader_save>      (*this);
    d->unsubscribe<events::shader::clear,                &render::on_shader_clear>     (*this);
    d->unsubscribe<events::shader::reload,               &render::on_shader_reload>    (*this);
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

void cots::graphics::render::on_shader_save(const events::shader::save& event)
{
    std::lock_guard lock(command_mutex_);
    pending_.shader_save = true;
}

void cots::graphics::render::on_shader_clear(const events::shader::clear& event)
{
    std::lock_guard lock(command_mutex_);
    pending_.shader_clear = true;
}

void cots::graphics::render::on_shader_reload(const events::shader::reload& event)
{
    std::lock_guard lock(command_mutex_);
    pending_.shader_reload     = true;
    pending_.shader_reload_key = event.key;
}

void cots::graphics::render::publish_snapshot()
{
    const std::uint32_t just_built = snapshots_.building_idx.load(std::memory_order_relaxed);

    //~ hand it to the RT
    snapshots_.pending_idx.store(just_built, std::memory_order_release);

    //~ pick a new building slot thats neither pending nor being rendered
    //  with 3 slots and at most 2 "taken" (pending + render) one is always free
    const std::uint32_t rendering = snapshots_.render_idx;   //~ read is racy but only used as a hint
    std::uint32_t next = (just_built + 1u) % 3u;
    if (next == rendering) next = (next + 1u) % 3u;

    snapshots_.building_idx.store(next, std::memory_order_relaxed);

    //~ clear the fresh building slot for this frames writes
    snapshots_.scene[next].clear();
}

bool cots::graphics::render::acquire_snapshot()
{
    const std::uint32_t pending =
        snapshots_.pending_idx.exchange(invalid_idx, std::memory_order_acquire);

    if (pending == invalid_idx)
        return false;   //~ nothing new - caller redraws render_idx_

    snapshots_.render_idx = pending;
    return true;
}
