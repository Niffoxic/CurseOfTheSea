//=============================================================================
// Curse of the Sea
//=============================================================================
// Created by  Niffoxic - Harsh Dubey
// Module      WM9M6 Fundamentals of Games Research Development and Management
// Institution University of Warwick
//
// A linear story driven pirate adventure built from scratch in C++23 and
// DirectX 12 for the University of Warwick game project assessment.
//=============================================================================
#include "trishul/renderer/render.h"
#include "trishul/event/window_event.h"
#include "trishul/event/render_event.h"
#include "trishul/event/dispatcher.h"
#include "trishul/core/engine_assert.h"
#include "trishul/core/engine_config.h"

#include <d3d12.h>
#include <thread>
#include <mutex>
#include <array>

#include "trishul/utils/logger.h"

//~ hardware
#include "trishul/core/dependency_handler.h"
#include "trishul/renderer/hardware/device.h"
#include "trishul/renderer/hardware/fence.h"

using namespace trishul::render;

struct graphics::impl
{
    //~ initialize hardware
    [[nodiscard]] bool init_bootstrap();

    //~ drawing lifecycle
    void draw_frame(const scene_snapshot& snapshot);
    void record_frame(
        std::uint32_t frame_id,
        const scene_snapshot& snapshot,
        std::vector<ID3D12CommandList*>& out
    );
    void submit_frame(const std::vector<ID3D12CommandList*>& prepared_lists) const;

    //~ handle events
    void process_pending_events();
    void subscribe_events      ();
    void unsubscribe_events    ();

    //~ event callbacks
    //~ windows events
    void on_window_resized (const events::window_resized& event);
    void on_display_changed(const events::window_display_changed& event);

    //~ device fired the gpu came back possibly on a different adapter
    void on_device_recreated(const events::device_recreated& event);

    //~ device vanished I will try to rebuild but its unsafe!
    void on_device_lost(const events::device_lost& event);

    //~ hardware lifecycle render thread side
    void rebuild_hardware      ();  //~ reinit any child that raised need_rebuild
    void build_capabilities    ();  //~ device into a pod snapshot
    void apply_display_settings(const display_settings& settings);

    //~ snapshots related stuff
    void publish_snapshot(); //~ on begin update
    bool acquire_snapshot(); //~ grabs pending if theres any that is

    //~ members
    //~ initialization related
    std::mutex        command_mutex_;
    std::atomic<bool> running_      { false };
    std::atomic<bool> render_ready_ { false };

    //~ follow up after initializing core hardware interfaces
    std::atomic<bool>          bootstrap_ready_ { false };
    std::atomic<std::uint32_t> warming_step_   { 0u };

    //~ hardware
    dependency_handler<hardware::interfaces> hardware_handler_{};
    hardware::device device_{};
    hardware::fence fence_  {};

    //~ later will be using it for main menu basically display settings
    //~ changes get accounted
    mutable std::mutex                          display_mutex_;
    std::shared_ptr<const display_capabilities> caps_;
    display_settings                            current_settings_{};
    display_settings                            desired_settings_ {};
    std::atomic<bool>                           settings_dirty_{ false };
    std::atomic<bool>                           outputs_dirty_ { false };
    std::atomic<bool>                           caps_dirty_    { false };

    //~ render related
    struct
    {
        std::array<scene_snapshot, config::RENDER_SCENE_SNAPSHOT> scene{};

        std::atomic<std::uint32_t>    building_idx{ 0 };
        std::atomic<std::uint32_t>    pending_idx { config::INVALID_INDEX };
        std::uint32_t                 render_idx  { 0 }; //~ RTs current slot

        std::uint64_t frame_counter{ 0 };   //~ MT snapshot id source

        scene_snapshot& latest()
        {
            return scene[render_idx];
        }

        scene_snapshot& next_build()
        {
            return scene[building_idx.load(std::memory_order_relaxed)];
        }
    } snapshots_;
};
#pragma region GRAPHICS

graphics::graphics()
: render_(std::make_unique<graphics::impl>())
{}

graphics::~graphics()
{

}

bool graphics::initialize()
{
    ENGINE_ASSERT_MSG(render_, "This is a huge error please delete your os!");
    render_->subscribe_events();
    render_->running_ = true;
    render_thread_ = std::thread(&graphics::render_entry, this);
    return true;
}

void graphics::deinitialize() noexcept
{
    if (not render_) return; //~ already deleted

    //~ stop the render thread FIRST so the device outlives every gpu call
    render_->running_ = false;

    if (render_thread_.joinable()) //~ signaled closure
    {
        render_thread_.join();
    }

    //~ thread is dead now safe to drop subscriptions and tear hardware down
    render_->unsubscribe_events();

    //~ deintialize whole hardwares
    for (auto it = render_->hardware_handler_.rbegin();
         it != render_->hardware_handler_.rend(); ++it)
    {
        if (*it) (*it)->deinitialize();
    }
}

void graphics::begin_update(float dt)
{

}

void graphics::end_update()
{

}

bool graphics::is_ready() const noexcept
{
    ENGINE_ASSERT_MSG(render_, "Is your renderer deleted already? did u call deinitialize?");
    return render_->running_.load(std::memory_order_acquire);
}

bool graphics::is_bootstrap_ready() const noexcept
{
    ENGINE_ASSERT_MSG(render_, "Is your renderer deleted already? did u call deinitialize?");
    return render_->bootstrap_ready_.load(std::memory_order_acquire);
}

scene_snapshot& graphics::building_snapshot() const noexcept
{
    ENGINE_ASSERT_MSG(render_, "Is your renderer deleted already? did u call deinitialize?");
    return render_->snapshots_.next_build();
}

graphics_fps graphics::frame_fps() const noexcept
{
    return {};
}

display_capabilities graphics::display_options() const
{
    ENGINE_ASSERT_MSG(render_, "renderer gone did you deinitialize already?");

    //~ grab the shared snapshot under the lock then copy outside it
    std::shared_ptr<const display_capabilities> snap;
    {
        std::lock_guard lock(render_->display_mutex_);
        snap = render_->caps_;
    }
    return snap ? *snap : display_capabilities{};
}

display_settings graphics::current_display_settings() const
{
    ENGINE_ASSERT_MSG(render_, "renderer gone did you deinitialize already?");
    std::lock_guard lock(render_->display_mutex_);
    return render_->current_settings_;
}

void graphics::request_display_settings(const display_settings& settings)
{
    ENGINE_ASSERT_MSG(render_, "renderer gone did you deinitialize already?");

    {
        std::lock_guard lock(render_->display_mutex_);
        render_->desired_settings_ = settings;
    }
    //~ render thread drains this in process_pending_events
    render_->settings_dirty_.store(true, std::memory_order_release);

    LOG_INFO("display change requested manual {} adapter {} output {}",
        settings.manual_adapter, settings.adapter_index, settings.output_index);
}

void graphics::render_entry() const
{
    ENGINE_ASSERT_MSG(render_, "This is a huge error please delete your os!");

    if (not render_->init_bootstrap())
    {
        render_->running_ = false;
        LOG_ERROR("Render Thread bootstrap is failed to initialize!");
        return;
    }

    render_->bootstrap_ready_.store(true, std::memory_order_release);
    LOG_INFO("[render] bootstrap is complete!");

    //~ thead properties
    ((void)SetThreadDescription(
        GetCurrentThread(),
        L"Trishul Renderer")
    );

    while (render_->running_.load(std::memory_order_relaxed))
    {
        render_->process_pending_events();
        render_->draw_frame(render_->snapshots_.latest());
    }
}

#pragma endregion

#pragma region GRAPHICS_IMPL

bool graphics::impl::init_bootstrap()
{
    //~ default boot auto picks the best gpu the user can switch at runtime
    //~ by using the graphics request_display_settings
    hardware::device_create_info device_info{};
    device_info.flags             = DXGI_ADAPTER_FLAG_SOFTWARE;
    device_info.preference        = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
    device_info.min_feature_level = D3D_FEATURE_LEVEL_12_0; //~ thats the lowest
    device_.set_config(device_info);

    //~ fence builds on the device and continues its timeline across swaps
    hardware::fence_config fence_info{};
    fence_info.dev           = &device_;
    fence_info.initial_value = 0u;
    fence_.set_config(fence_info);

    //~ register each hardware to the handler
    hardware_handler_.register_type(&device_);
    hardware_handler_.register_type(&fence_);
    
    //~ build dependencies betweem hardwares
    hardware_handler_.add_dependency<hardware::fence, hardware::device>();

    //~ initialize in correct order
    for (auto* hw : hardware_handler_)
    {
        if (not hw->initialize())
        {
            LOG_CRITICAL("hardware '{}' failed to initialize", hw->name());
            return false;
        }
        LOG_INFO("hardware '{}' online", hw->name());
    }

    //~ seed what we actually landed on then publish the first snapshot
    {
        std::lock_guard lock(display_mutex_);
        current_settings_.manual_adapter = device_info.manual;
        current_settings_.adapter_index  = device_.current_adapter_info().adapter_index;
    }
    build_capabilities();

    LOG_INFO("DirectX 12 Created and Running Just Fine!");
    return true;
}

void graphics::impl::draw_frame(const scene_snapshot &snapshot)
{

}

void graphics::impl::record_frame(
    std::uint32_t frame_id,
    const scene_snapshot &snapshot,
    std::vector<ID3D12CommandList *> &out
)
{

}

void graphics::impl::submit_frame(const std::vector<ID3D12CommandList*>& prepared_lists) const
{

}

void graphics::impl::process_pending_events()
{
    bool rebuild_caps = false;

    //~ monitor got plugged unplugged or changed mode rescan the outputs
    if (outputs_dirty_.exchange(false, std::memory_order_acquire)) //~ very rare what if!
    {
        device_.refresh_outputs();
        rebuild_caps = true;
    }

    //~ the menu asked for a different gpu monitor or mode
    if (settings_dirty_.exchange(false, std::memory_order_acquire))
    {
        display_settings desired;
        {
            std::lock_guard lock(display_mutex_);
            desired = desired_settings_;
        }
        apply_display_settings(desired);
    }

    //~ device_recreated arrived current adapter changed restamp the snapshot
    if (caps_dirty_.exchange(false, std::memory_order_acquire))
    {
        rebuild_caps = true;
    }

    //~ reinitialize any hardware that raised need_rebuild
    rebuild_hardware();

    if (rebuild_caps) build_capabilities();
}

void graphics::impl::rebuild_hardware()
{
    //~ parent to child order so a device rebuild lands before its dependents
    for (auto* hw : hardware_handler_)
    {
        if (not hw->need_rebuild()) continue;

        if (hw->initialize())
            LOG_INFO("hardware '{}' rebuilt", hw->name());
        else
            LOG_ERROR("hardware '{}' rebuild failed", hw->name());
    }
}

void graphics::impl::subscribe_events()
{
    auto* d = service_locator::try_get<events::dispatcher>();
    if (not d) return;

    d->subscribe<events::window_resized,         &impl::on_window_resized  >(*this);
    d->subscribe<events::window_display_changed, &impl::on_display_changed >(*this);
    d->subscribe<events::device_recreated,       &impl::on_device_recreated>(*this);
    d->subscribe<events::device_lost,            &impl::on_device_lost     >(*this);
}

void graphics::impl::unsubscribe_events()
{
    auto* d = service_locator::try_get<events::dispatcher>();
    if (not d) return;

    d->unsubscribe<events::window_resized,         &impl::on_window_resized  >(*this);
    d->unsubscribe<events::window_display_changed, &impl::on_display_changed >(*this);
    d->unsubscribe<events::device_recreated,       &impl::on_device_recreated>(*this);
    d->unsubscribe<events::device_lost,            &impl::on_device_lost     >(*this);
}

void graphics::impl::on_window_resized(const events::window_resized &event)
{

}

void graphics::impl::on_display_changed(const events::window_display_changed &event)
{
    //~ just a flag process message should resolve it in render thread later
    outputs_dirty_.store(true, std::memory_order_release);
}

void graphics::impl::on_device_recreated(const events::device_recreated &event)
{
    //~ runs on the dispatcher thread render thread re stamps the caps snapshot
    caps_dirty_.store(true, std::memory_order_release);
}

void graphics::impl::on_device_lost(const events::device_lost &event)
{
    device_.mark_for_rebuild();
}

void graphics::impl::build_capabilities()
{
    //~ copy device state into a plain snapshot
    auto caps = std::make_shared<display_capabilities>();
    caps->current_adapter_index = device_.current_adapter_info().adapter_index;

    for (const auto& a : device_.adapters_info())
    {
        caps->adapters.push_back(adapter_option{
            a.adapter_index,
            a.name,
            a.dedicated_video_memory,
            a.vendor_id,
            a.device_id,
            a.is_wrap,
        });
    }

    for (const auto& o : device_.outputs())
    {
        output_option out{};
        out.index          = o.index;
        out.name           = o.device_name;
        out.desktop_width  = o.desktop_width;
        out.desktop_height = o.desktop_height;
        out.is_primary     = o.is_primary;

        out.modes.reserve(o.supported_modes.size());
        for (const auto& m : o.supported_modes)
        {
            out.modes.push_back(display_mode{
                m.width, m.height,
                m.refresh_numerator, m.refresh_denominator,
            });
        }
        caps->outputs.push_back(std::move(out));
    }

    //~ publish the finished snapshot
    std::lock_guard lock(display_mutex_);
    caps_ = std::move(caps);
}

void graphics::impl::apply_display_settings(const display_settings& settings)
{
    //~ only a gpu swap needs a full device recreate rest is gonna be swapchain side
    const std::uint32_t current = device_.current_adapter_info().adapter_index;
    if (settings.manual_adapter && settings.adapter_index != current)
    {
        hardware::device_create_info info{};
        info.manual              = true;
        info.adapter_index       = settings.adapter_index;
        info.preference          = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
        info.min_feature_level   = D3D_FEATURE_LEVEL_12_0;
        info.allow_warp_fallback = true;

        LOG_INFO("switching gpu to adapter {}", settings.adapter_index);
        if (not device_.recreate(info))
        {
            //~ recreate already fired device_recreate_failed menu can react
            LOG_ERROR("gpu switch failed staying where we can");
        }
    }

    //~ output mode fullscreen vsync are swapchain state stored to wire later when I create swapchain
    {    //~ caps refresh on a gpu switch rides the device_recreated
        std::lock_guard lock(display_mutex_);
        current_settings_               = settings;
        current_settings_.adapter_index = device_.current_adapter_info().adapter_index;
    }
}

bool graphics::impl::acquire_snapshot()
{
    const std::uint32_t pending =
        snapshots_.pending_idx.exchange(config::INVALID_INDEX, std::memory_order_acquire);

    if (pending == config::INVALID_INDEX) return false; //~ nothing new

    snapshots_.render_idx = pending; //~ got a new frame to draw
    return true;
}

void graphics::impl::publish_snapshot()
{
    const std::uint32_t just_built = snapshots_.building_idx.load(std::memory_order_relaxed);

    //~ hand it to the render thread
    snapshots_.pending_idx.store(just_built, std::memory_order_release);

    //~ pick the idle render slot so that main thread can have it
    const std::uint32_t active_render_slot = snapshots_.render_idx;
    std::uint32_t next = (just_built + 1u) % config::RENDER_SCENE_SNAPSHOT;

    if (next == active_render_slot) //~ already being drawn by render thread
    {
        next = (just_built + 1u) % config::RENDER_SCENE_SNAPSHOT;
    }
    //~ safe to hand over
    snapshots_.building_idx.store(next, std::memory_order_release);
    snapshots_.scene[next].clear();
}

#pragma endregion
