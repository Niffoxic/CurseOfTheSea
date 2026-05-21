// Created by Niffoxic (Harsh Dubey)
#include "engine/engine.h"
#include <spdlog/spdlog.h>

#include <ranges>

#include "engine/core/cots_assert.h"
#include "engine/events/graphics_event.h"
#include "engine/system/define_features.h"
#include "engine/platform/platform_windows.h"
#include "engine/graphics/render.h"

#include "engine/utils/profiler.h"

#define REGISTER_FEATURE_TO_SCHEDULER(scheduler, feature_class) \
do { \
auto _cots_feat = ::cots::feature::locator::resolve<feature_class>(); \
(scheduler).register_type(std::ref(_cots_feat)); \
} while(0)


cots::engine::engine() = default;

cots::engine::~engine()
{
    for (const auto subsystem: std::views::reverse(subsystem_scheduler_))
    {
        if (subsystem) subsystem->deinitialize();
    }
}

bool cots::engine::init()
{
    initialize_features();
    regulate_subsystems();
    regulate_tickable  ();

    //~ defaults
    timer_->reset();
    timer_->set_target_frame_ps(360);

    return true;
}

void cots::engine::tick()
{
    COTS_PROFILE_SCOPE("engine::tick");
    timer_->step();
    test_fps();
    test_debug_input();
    update_tickable();
}

bool cots::engine::should_close() const noexcept
{
    return windows_->should_close();
}

float cots::engine::delta_time() const noexcept
{
    return timer_->delta_time();
}

void cots::engine::initialize_features()
{
    timer_      = feature::locator::resolve<utils::timer>();

    //~ setup subsystems
    windows_ = feature::locator::resolve<platform::windows>();
    windows_->setup_config(reinterpret_cast<const std::byte*>(&config_manager_.windows_config()));
}

void cots::engine::regulate_subsystems()
{
    REGISTER_FEATURE_TO_SCHEDULER(subsystem_scheduler_, audio::system);

    subsystem_scheduler_.register_type(std::ref(windows_));

    auto render  = feature::locator::resolve<graphics::render>();
    subsystem_scheduler_.register_type(std::ref(render));

    //~ configure dependencies
    subsystem_scheduler_.add_dependency(render, windows_);

    for (const auto subsystem: subsystem_scheduler_)
    {
        if (not subsystem->initialize())
        {
            spdlog::error("Failed to initialize subsystem");
            COTS_FAIL_MSG("Failed to initialize subsystem");
        }
    }
    spdlog::info("All Subsystem initialized");
}

void cots::engine::regulate_tickable()
{
    REGISTER_FEATURE_TO_SCHEDULER(tickable_scheduler_, audio::system);
    REGISTER_FEATURE_TO_SCHEDULER(tickable_scheduler_, events::dispatcher);

    render_ = feature::locator::resolve<graphics::render>();

    tickable_scheduler_.register_type(std::ref(windows_));
    tickable_scheduler_.register_type(std::ref(render_));

    //~ dependencies
    tickable_scheduler_.add_dependency(render_, windows_);
}

void cots::engine::update_tickable()
{
    const float dt = timer_->delta_time();

    //~ update begin
    for (const auto tickable: tickable_scheduler_)
    {
        if (tickable)
        {
            tickable->begin_update(dt);
        }
    }

    //~ update end
    for (const auto iter : std::views::reverse(tickable_scheduler_))
    {
        if (iter)
        {
            iter->end_update();
        }
    }
}

void cots::engine::test_debug_input() const
{
    const auto& kb = windows_->keyboard;
    const auto dispatcher = feature::locator::resolve<events::dispatcher>();

    using mode = graphics::hardware::display_mode;
    namespace req = events::swapchain;

    // toggle borderless <-> windowed
    if (kb.pressed(VK_F11))
    {
        const auto cur = render_->swapchain().current_mode();
        dispatcher->enqueue<req::set_display_mode>(
            cur == mode::windowed ? mode::borderless : mode::windowed);
    }

    if (kb.pressed(VK_F10)) dispatcher->enqueue<req::set_display_mode>(mode::exclusive_fullscreen);
    if (kb.pressed(VK_F9))  dispatcher->enqueue<req::set_display_mode>(mode::windowed);

    if (kb.pressed('1')) dispatcher->enqueue<req::set_windowed_size>(1280u, 720u);
    if (kb.pressed('2')) dispatcher->enqueue<req::set_windowed_size>(1600u, 900u);
    if (kb.pressed('3')) dispatcher->enqueue<req::set_windowed_size>(1920u, 1080u);

    //~ shader event tests
    namespace sh = events::shader;
    if (kb.pressed(VK_F5)) dispatcher->enqueue<sh::reload>(std::uint64_t{0});
    if (kb.pressed(VK_F6)) dispatcher->enqueue<sh::save>();
    if (kb.pressed(VK_F7)) dispatcher->enqueue<sh::clear>();
}

void cots::engine::test_fps() const
{
    using clock = std::chrono::steady_clock;

    static auto window_start = clock::now();
    static int  mt_frames    = 0;
    ++mt_frames;

    const auto  now     = clock::now();
    const float elapsed = std::chrono::duration<float>(now - window_start).count();

    if (elapsed >= 0.5f)
    {
        const float mt_fps = static_cast<float>(mt_frames) / elapsed;
        const float mt_ms  = (elapsed * 1000.f) / static_cast<float>(mt_frames);

        const float rt_fps = render_->fps();
        const float rt_ms  = render_->frame_ms();

        windows_->set_debug(std::format(
            "MT {:.0f} fps ({:.2f} ms)  |  RT {:.0f} fps ({:.2f} ms)",
            mt_fps, mt_ms, rt_fps, rt_ms));

        window_start = now;
        mt_frames    = 0;
    }
}
