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
#include "trishul/utils/timer.h"

#include <d3d12.h>
#include <D3D12MemAlloc.h>
#include <thread>
#include <mutex>
#include <array>

#include "trishul/utils/logger.h"

//~ hardware
#include "trishul/core/dependency_handler.h"
#include "trishul/renderer/hardware/device.h"
#include "trishul/renderer/hardware/fence.h"
#include "trishul/renderer/hardware/swapchain.h"
#include "trishul/renderer/hardware/descriptor_heap.h"
#include "trishul/renderer/hardware/queue_timeline.h"
#include "trishul/renderer/hardware/depth_target.h"
#include "trishul/renderer/hardware/upload_arena.h"
#include "trishul/renderer/hardware/deferred_releaser.h"
#include "trishul/renderer/hardware/texture_manager.h"
#include "trishul/platform/platform_windows.h"

using namespace trishul::render;

namespace
{
    //~ the users speaks window_mode the swapchain speaks display_mode
    //~ types bridge the two so game side settings drive the hardware
    hardware::display_mode to_hw_display_mode(const window_mode mode) noexcept
    {
        switch (mode)
        {
        case window_mode::borderless: return hardware::display_mode::borderless;
        case window_mode::exclusive:  return hardware::display_mode::exclusive_fullscreen;
        case window_mode::windowed:
        default:                      return hardware::display_mode::windowed;
        }
    }
} // namespace

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
    void on_window_resized   (const events::window_resized& event);
    void on_display_changed  (const events::window_display_changed& event);

    //~ device fired the gpu came back possibly on a different adapter
    void on_device_recreated(const events::device_recreated& event);

    //~ device vanished I will try to rebuild but its unsafe!
    void on_device_lost(const events::device_lost& event);

    //~ hardware lifecycle render thread side
    void rebuild_hardware      ();  //~ in case some events happens which corrupts whole resources
    void deinit_dependents     () const;  //~ from child to the parent
    void init_dependents       () const;  //~ from parent to the child
    void build_capabilities    ();
    void apply_display_settings(const display_settings& settings);

    //~ refresh the gpu memory and pool stats snapshot render thread side
    void build_gpu_stats       ();

    //~ measure one render loop iteration and publish fps ms render thread side
    void tick_frame_timing     ();

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
    hardware::device          device_           {};
    hardware::fence           fence_            {};
    hardware::swapchain       swapchain_        {};
    hardware::descriptor_heap bindless_         {};
    hardware::graphics_timeline graphics_timeline_{};
    hardware::compute_timeline  compute_timeline_ {};
    hardware::copy_timeline     copy_timeline_    {};
    hardware::depth_target      depth_            {};
    hardware::upload_arena      uploader_         {};
    hardware::deferred_releaser releaser_         {};
    hardware::texture_manager   textures_         {};
    //~ later will be using it for main menu basically display settings
    //~ changes get accounted
    mutable std::mutex                          display_mutex_;
    std::shared_ptr<const display_capabilities> caps_;
    display_settings                            current_settings_{};
    display_settings                            desired_settings_ {};
    std::atomic<bool>                           settings_dirty_{ false };
    std::atomic<bool>                           outputs_dirty_ { false };
    std::atomic<bool>                           caps_dirty_    { false };

    //~ windows driven resize fullscreen
    std::atomic<bool>                           resize_pending_{ false };
    std::atomic<std::uint32_t>                  pending_width_ { 0u };
    std::atomic<std::uint32_t>                  pending_height_{ 0u };

    //~ gpu memory and pool stats render thread fills it main thread reads it
    mutable std::mutex                          stats_mutex_;
    gpu_stats                                   gpu_stats_{};
    std::uint32_t                               stats_tick_{ 0u };

    timer                                       frame_timer_{};
    float                                       frame_ms_avg_{ 0.f };
    std::atomic<int>                            render_fps_  { 0 };
    std::atomic<int>                            render_ms_   { 0 };

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
    if (not render_) return {};
    return 
    {
        render_->render_fps_.load(std::memory_order_relaxed),
        render_->render_ms_ .load(std::memory_order_relaxed)
    };
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

gpu_stats graphics::gpu_statistics() const
{
    ENGINE_ASSERT_MSG(render_, "renderer gone did you deinitialize already?");
    std::lock_guard lock(render_->stats_mutex_);
    return render_->gpu_stats_;
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
    LOG_INFO("bootstrap is complete!");

    //~ thead properties
    ((void)SetThreadDescription(
        GetCurrentThread(),
        L"Trishul Renderer")
    );

    //~ start the render clock 
    render_->frame_timer_.reset();
    render_->frame_timer_.set_target_fps(2048); //~ default max render fps
    while (render_->running_.load(std::memory_order_relaxed))
    {
        render_->tick_frame_timing();
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

    //~ bindless cbv srv uav heap also builds straight on the device
    hardware::descriptor_heap_config heap_info{};
    heap_info.dev      = &device_;
    heap_info.capacity = config::BINDLESS_CAPACITY;
    bindless_.set_config(heap_info);

    //~ queue timelines each owns its fence the queue kind is baked into the
    //~ type so that they all share one config just the device
    hardware::queue_timeline_config timeline_info{};
    timeline_info.dev = &device_;
    graphics_timeline_.set_config(timeline_info);
    compute_timeline_ .set_config(timeline_info);
    copy_timeline_    .set_config(timeline_info);

    //~ swapchain needed window handle must be ready since
    //~ renderer depends upon windows platform
    HWND          hwnd   = nullptr;
    std::uint32_t back_w = 1280u;
    std::uint32_t back_h = 720u;
    if (auto* window = service_locator::try_get<platform_window>())
    {
        hwnd = window->get_window_handle();
        if (const auto sz = window->get_window_size<std::uint32_t>();
            sz.width > 0u && sz.height > 0u)
        {
            back_w = sz.width;
            back_h = sz.height;
        }
    }

    //~ depth target matches the backbuffer and samples through the bindless heap
    hardware::depth_target_config depth_info{};
    depth_info.dev      = &device_;
    depth_info.bindless = &bindless_;
    depth_info.width    = back_w;
    depth_info.height   = back_h;
    depth_.set_config(depth_info);

    //~ upload arena batches gpu uploads on the copy queue builds on the device
    hardware::upload_arena_config upload_info{};
    upload_info.dev = &device_;
    uploader_.set_config(upload_info);

    //~ deferred releaser returns freed descriptor slots to the bindless heap
    hardware::deferred_releaser_config releaser_info{};
    releaser_info.bindless = &bindless_;
    releaser_.set_config(releaser_info);

    //~ texture manager uploads through the arena registers
    //~ srvs in the bindless heap and defers its frees through the releaser
    hardware::texture_manager_config texture_info{};
    texture_info.dev      = &device_;
    texture_info.bindless = &bindless_;
    texture_info.releaser = &releaser_;
    texture_info.arena    = &uploader_;
    textures_.set_config(texture_info);

    //~ register each hardware to the handler
    hardware_handler_.register_type(&device_);
    hardware_handler_.register_type(&fence_);
    hardware_handler_.register_type(&bindless_);
    hardware_handler_.register_type(&graphics_timeline_);
    hardware_handler_.register_type(&compute_timeline_);
    hardware_handler_.register_type(&copy_timeline_);
    hardware_handler_.register_type(&depth_);
    hardware_handler_.register_type(&uploader_);
    hardware_handler_.register_type(&releaser_);
    hardware_handler_.register_type(&textures_);

    if (hwnd) //~ god knows who is playing my game without a SCREEN!
    {
        hardware::swapchain_config swap_info{};
        swap_info.dev                = &device_;
        swap_info.info.window_handle = hwnd;
        swap_info.info.width         = back_w;
        swap_info.info.height        = back_h;
        swap_info.info.mode          = hardware::display_mode::windowed;
        swap_info.info.frame_count   = config::SWAPCHAIN_BUFFER_COUNT;
        swapchain_.set_config(swap_info);

        hardware_handler_.register_type(&swapchain_);
    }
    else
    {
        LOG_WARN("no window handle swapchain not registered");
    }

    //~ build dependencies
    hardware_handler_.add_dependency<hardware::fence,             hardware::device>();
    hardware_handler_.add_dependency<hardware::descriptor_heap,   hardware::device>();
    hardware_handler_.add_dependency<hardware::graphics_timeline, hardware::device>();
    hardware_handler_.add_dependency<hardware::compute_timeline,  hardware::device>();
    hardware_handler_.add_dependency<hardware::copy_timeline,     hardware::device>();
    hardware_handler_.add_dependency<hardware::depth_target,      hardware::device>();
    hardware_handler_.add_dependency<hardware::depth_target,      hardware::descriptor_heap>();
    hardware_handler_.add_dependency<hardware::upload_arena,      hardware::device>();
    //~ releaser frees allocations and returns slots so it tears down before anything elses
    hardware_handler_.add_dependency<hardware::deferred_releaser, hardware::device>();
    hardware_handler_.add_dependency<hardware::deferred_releaser, hardware::descriptor_heap>();
    //~ textures come up last and tear down first they touch all four below the
    //~ device and heap must outlive its frees the arena and releaser must exist
    //~ before any texture is created or destroyed or will be having nightmare deadling with
    //~ memory allocator
    hardware_handler_.add_dependency<hardware::texture_manager,   hardware::device>();
    hardware_handler_.add_dependency<hardware::texture_manager,   hardware::descriptor_heap>();
    hardware_handler_.add_dependency<hardware::texture_manager,   hardware::upload_arena>();
    hardware_handler_.add_dependency<hardware::texture_manager,   hardware::deferred_releaser>();
    if (hwnd) //~ very very rare to not have this!
        hardware_handler_.add_dependency<hardware::swapchain, hardware::device>();

    //~ initialize in correct order
    for (auto* hw: hardware_handler_)
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
    build_gpu_stats   ();

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

    //~ windows driven resize fullscreen toggle or resolution change the
    //~ backbuffers gotta be matching the new client size before we draw again
    if (resize_pending_.exchange(false, std::memory_order_acquire))
    {
        const std::uint32_t w = pending_width_ .load(std::memory_order_relaxed);
        const std::uint32_t h = pending_height_.load(std::memory_order_relaxed);

        if (swapchain_.dxgi_swapchain() && w > 0u && h > 0u &&
            (w != swapchain_.width() || h != swapchain_.height()))
        {
            if (swapchain_.resize(w, h))
            {
                //~ the depth target shares the render resolution keeping it matching
                (void)depth_.resize(w, h);

                LOG_INFO("swapchain and depth handled window resize {}x{}", w, h);
                std::lock_guard lock(display_mutex_);
                current_settings_.width  = w;
                current_settings_.height = h;
            }
            else
            {
                LOG_ERROR("swapchain resize to {}x{} failed", w, h);
            }
        }
    }

    if (rebuild_caps) build_capabilities();

    //~ refresh the stats snapshot every so often not on every spin its only
    //~ for a debug overlay or the menu so coarse freshness is plenty anyways
    if ((stats_tick_++ & 0x3Fu) == 0u) build_gpu_stats();
}

void graphics::impl::build_gpu_stats()
{
    gpu_stats s{};

    //~ d3d12ma carries both the os budget and its own block allocation totals
    //~ getbudget is the cheap cached query unlike a full statistics walk
    if (auto* alloc = device_.allocator())
    {
        D3D12MA::Budget local{};
        D3D12MA::Budget nonlocal{};
        alloc->GetBudget(&local, &nonlocal);

        constexpr double to_mb = 1.0 / (1024.0 * 1024.0);

        s.memory.local_budget_mb    = static_cast<double>(local.BudgetBytes)    * to_mb;
        s.memory.local_usage_mb     = static_cast<double>(local.UsageBytes)     * to_mb;
        s.memory.nonlocal_budget_mb = static_cast<double>(nonlocal.BudgetBytes) * to_mb;
        s.memory.nonlocal_usage_mb  = static_cast<double>(nonlocal.UsageBytes)  * to_mb;
        s.memory.allocated_mb       = static_cast<double>(local.Stats.AllocationBytes + nonlocal.Stats.AllocationBytes) * to_mb;
        s.memory.block_mb           = static_cast<double>(local.Stats.BlockBytes      + nonlocal.Stats.BlockBytes)      * to_mb;
        s.memory.allocation_count   = local.Stats.AllocationCount + nonlocal.Stats.AllocationCount;
        s.memory.block_count        = local.Stats.BlockCount      + nonlocal.Stats.BlockCount;
    }

    //~ upload arena pool churn the render thread owns these so reading here is safe
    s.upload.free_staging      = uploader_.free_count();
    s.upload.in_flight_staging = uploader_.in_flight_count();
    s.upload.reused            = uploader_.reused_count();
    s.upload.allocated         = uploader_.allocated_count();

    //~ how many resources are parked in the deferred release queue
    s.deferred_pending = releaser_.pending_count();

    std::lock_guard lock(stats_mutex_);
    gpu_stats_ = s;
}

void graphics::impl::tick_frame_timing()
{
    frame_timer_.step();
    const float dt_ms = frame_timer_.delta_time_ms();

    frame_ms_avg_ = (frame_ms_avg_ <= 0.f)
        ? dt_ms
        : frame_ms_avg_ * 0.9f + dt_ms * 0.1f;

    const int ms  = static_cast<int>(frame_ms_avg_ + 0.5f);
    const int fps = (frame_ms_avg_ > 0.f)
        ? static_cast<int>(1000.f / frame_ms_avg_ + 0.5f)
        : 0;

    render_fps_.store(fps, std::memory_order_relaxed);
    render_ms_ .store(ms,  std::memory_order_relaxed);
}

void graphics::impl::rebuild_hardware()
{
    //~ only a device loss can force rebuilding
    if (not device_.need_rebuild()) return;

    LOG_WARN("device flagged for rebuild recreating the hardware graph");
    deinit_dependents();
    if (not device_.recreate()) //~ reuses the last good config
        LOG_ERROR("device recreate failed");
    init_dependents();
}

void graphics::impl::deinit_dependents() const
{
    // from leaf to parent node
    for (auto it = hardware_handler_.rbegin(); it != hardware_handler_.rend(); ++it)
    {
        if (*it && *it != &device_) (*it)->deinitialize();
    }
}

void graphics::impl::init_dependents() const
{
    //~ from root to child
    for (auto* hw : hardware_handler_)
    {
        if (hw && hw != &device_ && not hw->initialize())
            LOG_ERROR("'{}' failed to rebuild after device recreate", hw->name());
    }
}

void graphics::impl::subscribe_events()
{
    auto* d = service_locator::try_get<events::dispatcher>();
    if (not d) return;

    d->subscribe<events::window_resized,            &impl::on_window_resized   >(*this);
    d->subscribe<events::window_display_changed,    &impl::on_display_changed  >(*this);
    d->subscribe<events::device_recreated,          &impl::on_device_recreated >(*this);
    d->subscribe<events::device_lost,               &impl::on_device_lost      >(*this);
}

void graphics::impl::unsubscribe_events()
{
    auto* d = service_locator::try_get<events::dispatcher>();
    if (not d) return;

    d->unsubscribe<events::window_resized,            &impl::on_window_resized   >(*this);
    d->unsubscribe<events::window_display_changed,    &impl::on_display_changed  >(*this);
    d->unsubscribe<events::device_recreated,          &impl::on_device_recreated >(*this);
    d->unsubscribe<events::device_lost,               &impl::on_device_lost      >(*this);
}

void graphics::impl::on_window_resized(const events::window_resized &event)
{
    //~ gonna be doing that on command process
    pending_width_ .store(event.width,  std::memory_order_relaxed);
    pending_height_.store(event.height, std::memory_order_relaxed);
    resize_pending_.store(true,         std::memory_order_release);
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

        //~ free dependents first so depth allocations descriptor slots and the
        //~ like are released while the old device and allocator still live then
        //~ recreate the device then rebuild them on the fresh one
        deinit_dependents(); //~ caused me a headache
        if (not device_.recreate(info))
        {
            //~ recreate already fired device_recreate_failed menu can react
            LOG_ERROR("gpu switch failed staying where we can");
        }
        init_dependents();
    }

    //~ its up a gpu switch above already rebuilt anyways
    if (swapchain_.dxgi_swapchain())
    {
        switch (to_hw_display_mode(settings.mode))
        {
        case hardware::display_mode::windowed:
            if (swapchain_.current_mode() != hardware::display_mode::windowed)
                (void)swapchain_.set_display_mode(hardware::display_mode::windowed);
            if (settings.width > 0u && settings.height > 0u &&
                (settings.width  != swapchain_.width() ||
                 settings.height != swapchain_.height()))
                (void)swapchain_.set_windowed_size(settings.width, settings.height);
            break;

        case hardware::display_mode::borderless:
            if (swapchain_.current_mode() != hardware::display_mode::borderless)
                (void)swapchain_.set_display_mode(hardware::display_mode::borderless);
            break;

        case hardware::display_mode::exclusive_fullscreen:
        {
            hardware::display_format fmt{};
            fmt.width               = settings.width;
            fmt.height              = settings.height;
            fmt.refresh_numerator   = settings.refresh_numerator;
            fmt.refresh_denominator = settings.refresh_denominator;
            (void)swapchain_.set_exclusive_mode(settings.output_index, fmt);
            break;
        }
        }
    }

    //~ the applied settings vsync gonna be landing on the present
    {
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
