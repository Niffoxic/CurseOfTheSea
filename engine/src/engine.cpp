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
#include "trishul/engine.h"
#include "trishul/core/interface/subsystems.h"
#include "trishul/core/interface/tickable.h"
#include "trishul/core/dependency_handler.h"
#include "trishul/core/engine_assert.h"
#include "trishul/utils/logger.h"
#include "trishul/utils/timer.h"

#include "trishul/services.h"

#include <windows.h>

#include "trishul/renderer/hardware/device.h"

using namespace trishul;

struct engine::impl
{
    explicit impl(engine_create_info info)
    : create_info_(std::move(info))
    {}

    //~ initialization
                 void initialize_services();
    [[nodiscard]]bool regulate_services  ();
                 void regulate_tickable  ();

    //~ per frame
    void update_tickable(float dt);
    void compute_fps    (float dt);

    //~ members
    engine_create_info create_info_;
    dependency_handler<interfaces::subsystems> subsystem_scheduler_;
    dependency_handler<interfaces::tickable>   tickable_scheduler_;

    timer          frame_timer_{};
    fps_information fps_{};
    float          fps_accum_time_  { 0.f };
    std::uint32_t  fps_accum_frames_{ 0u };

    //~ services
    platform_window*  window_ = nullptr;
    timer_manager*    timers_ = nullptr;
    render::graphics* render_ = nullptr;

    //~ loop thread priority saved for restore on shutdown
    int  prev_thread_priority_   { THREAD_PRIORITY_NORMAL };
    bool thread_priority_raised_ { false };
};

#pragma region ENGINE

engine::engine(engine_create_info info)
: p_(std::make_unique<impl>(std::move(info)))
{
    logger::instance().initialize();
    LOG_INFO("engine created window {}x{} icon id {}",
        p_->create_info_.window_width,
        p_->create_info_.window_height,
        p_->create_info_.icon_resource_id);
}

engine::~engine()
{
    if (not p_) return;

    LOG_INFO("engine shutting down");
    for (const auto subsystem : std::views::reverse(p_->subsystem_scheduler_))
    {
        if (subsystem)
        {
            LOG_DEBUG("deinitializing subsystem {}", subsystem->name());
            subsystem->deinitialize();
        }
    }

    //~ restore the loop thread priority we raised at startup
    if (p_->thread_priority_raised_ &&
        p_->prev_thread_priority_ != THREAD_PRIORITY_ERROR_RETURN)
    {
        SetThreadPriority(GetCurrentThread(), p_->prev_thread_priority_);
        LOG_DEBUG("loop thread priority restored");
    }

    logger::instance().deinitialize();
}

bool engine::initialize() const
{
    ENGINE_ASSERT_MSG(p_, "Corrupted Engine or destroyed already");
    LOG_INFO("engine initialize begin");

    //~ keep the loop thread ahead of normal background work for steady pacing
    p_->prev_thread_priority_ = GetThreadPriority(GetCurrentThread());
    if (SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL))
    {
        p_->thread_priority_raised_ = true;
        LOG_DEBUG("loop thread priority raised to above normal");
    }
    else
    {
        LOG_WARN("failed to raise loop thread priority");
    }

    p_->initialize_services();

    if (not p_->regulate_services())
    {
        LOG_CRITICAL("engine initialize failed during service regulation");
        return false;
    }

    p_->regulate_tickable();

    //~ start the clock fresh so the first frame dt is small
    p_->frame_timer_.reset();
    LOG_INFO("engine initialize complete");
    return true;
}

void engine::tick() const
{
    ENGINE_ASSERT_MSG(p_, "Corrupted Engine or destroyed already");

    //~ pace the frame and measure the real delta
    p_->frame_timer_.step();
    const float dt = p_->frame_timer_.delta_time();

    p_->compute_fps(dt);
    if (p_->timers_) p_->timers_->tick(dt);
    p_->update_tickable(dt);
}

bool engine::should_close() const noexcept
{
    ENGINE_ASSERT_MSG(p_, "Corrupted Engine or destroyed already");
    ENGINE_ASSERT_MSG(p_->window_, "Corrupted window or destroyed already");
    return p_->window_->should_close();
}

float engine::delta_time() const noexcept
{
    ENGINE_ASSERT_MSG(p_, "Corrupted Engine or destroyed already");
    return p_->frame_timer_.delta_time();
}

fps_information engine::get_fps() const noexcept
{
    ENGINE_ASSERT_MSG(p_, "Corrupted Engine or destroyed already");
    return p_->fps_;
}

#pragma endregion

#pragma region ENGINE_IMPLEMENTATION

void engine::impl::initialize_services()
{
    LOG_DEBUG("acquiring services");
    window_ = service_locator::get<platform_window> ();
    timers_ = service_locator::get<timer_manager>   ();
    render_ = service_locator::get<render::graphics>();

    frame_timer_.set_target_fps(create_info_.target_fps);
    if (create_info_.target_fps == 0u) LOG_DEBUG("frame cap uncapped");
    else                               LOG_DEBUG("frame cap {} fps", create_info_.target_fps);

    window_create_info window_info{};
    window_info.window_title     = create_info_.window_title;
    window_info.window_size      = win_size<int>{
        static_cast<int>(create_info_.window_width),
        static_cast<int>(create_info_.window_height) };
    window_info.icon_resource_id = create_info_.icon_resource_id;
    window_info.icon_path        = create_info_.icon_path;
    window_->set_window_create_info(window_info);

    LOG_DEBUG("window config applied {}x{}",
        create_info_.window_width, create_info_.window_height);

    //~ TODO: gotta initialize renderer! (gotta arch a smooth way of providing HWND without
    // giving access to whole platform windows)
}

bool engine::impl::regulate_services()
{
    //~ register subsystems
    LOG_DEBUG("registering subsystems");
    subsystem_scheduler_.register_type(window_);
    subsystem_scheduler_.register_type(render_);

    //~ build dependencies between subsystems
    subsystem_scheduler_.add_dependency(render_, window_); //~ renderer depends on windows

    //~ initialize
    for (auto* subsystem: subsystem_scheduler_)
    {
        ENGINE_ASSERT_MSG(subsystem, "Did you provide services?");
        LOG_INFO("initializing subsystem {}", subsystem->name());
        if (not subsystem->initialize())
        {
            LOG_ERROR("subsystem {} failed to initialize", subsystem->name());
            return false;
        }
    }
    LOG_INFO("all subsystems initialized");
    return true;
}

void engine::impl::regulate_tickable() //~ main thread tickables only
{
    //~ register and build dependencies
    LOG_DEBUG("registering tickables");
    tickable_scheduler_.register_type(window_);
    tickable_scheduler_.register_type(render_);

    auto* dispatcher = service_locator::get<events::dispatcher>();
    ENGINE_ASSERT_MSG(dispatcher, "Dispatcher isn't active this is a major problem");
    tickable_scheduler_.register_type(dispatcher);

    //~ window pumps messages first then the dispatcher flushes the queue
    tickable_scheduler_.add_dependency(dispatcher, window_);

    //~ snapshot handling will be provided later after every subsystem is done
    tickable_scheduler_.add_dependency(render_, window_); //~ render depends upon everything for MT tickable
}

void engine::impl::update_tickable(const float dt)
{
    //~ update begin
    for (const auto tickable: tickable_scheduler_)
    {
        if (tickable) tickable->begin_update(dt);
    }

    //~ update end
    for (const auto iter : std::views::reverse(tickable_scheduler_))
    {
        if (iter) iter->end_update();
    }
}

void engine::impl::compute_fps(const float dt)
{
    //~ average over half a second from the real frame delta
    fps_accum_time_ += dt;
    ++fps_accum_frames_;

    if (fps_accum_time_ >= 0.5f)
    {
        fps_.main_thread    = static_cast<std::uint32_t>(
            static_cast<float>(fps_accum_frames_) / fps_accum_time_ + 0.5f);
        fps_.main_thread_ms = (fps_accum_time_ * 1000.f)
            / static_cast<float>(fps_accum_frames_);

        fps_accum_time_   = 0.f;
        fps_accum_frames_ = 0u;

        //~ render render stats
        render::gpu_stats gpu_stats = render_->gpu_statistics();
        render::graphics_fps rt_fps = render_->frame_fps();

        const std::string message = std::format(
            "MT fps {} {:.3f} ms | RT fps {} {} ms | VRAM {:.1f}/{:.1f} MB",
            fps_.main_thread, fps_.main_thread_ms,
            rt_fps.fps, rt_fps.ms,
            gpu_stats.memory.local_usage_mb, gpu_stats.memory.local_budget_mb);

        window_->set_debug(message);
    }
}

#pragma endregion
