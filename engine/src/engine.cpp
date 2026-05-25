// Created by Niffoxic (Harsh Dubey)
#include "engine/engine.h"

#include <ranges>

#include "engine/core/cots_assert.h"
#include "engine/events/graphics_event.h"
#include "engine/system/define_features.h"
#include "engine/platform/platform_windows.h"
#include "engine/graphics/render.h"
#include "engine/core/framework/dependency_builder.h"

#define REGISTER_FEATURE_TO_SCHEDULER(scheduler, feature_class) \
do { \
auto _cots_feat = ::cots::feature::locator::resolve<feature_class>(); \
(scheduler).register_type(std::ref(_cots_feat)); \
} while(0)

using namespace cots;

class engine::implementation
{
public:
     implementation() = default;
    ~implementation();

    implementation(const implementation&) = delete;
    implementation(implementation&&)      = delete;

    implementation& operator=(const implementation&) = delete;
    implementation& operator=(implementation&&)      = delete;

    //~ life time operations
    bool  initialize  ();
    void  deinitialize();
    void  update      ();
    bool  should_close() const;
    float delta_time  () const;

    fps_stats get_fps_stats() const noexcept;

    void set_target_fps(std::uint32_t target_fps) const;

private:
    void initialize_subsystems();
    bool configure_subsystems ();
    void configure_tickables  ();
    void update_tickables     ();

    //~ debugging stuff
    void compute_fps();
    void display_fps();

private:
    config::manager config_manager_{};

    std::shared_ptr<utils::timer>      timer_  { nullptr };
    std::shared_ptr<platform::windows> windows_{ nullptr };
    std::shared_ptr<graphics::render>  render_ { nullptr };

    utils::dependency_scheduler<interfaces::subsystem> subsystem_;
    utils::dependency_scheduler<interfaces::tickable>  tickables_;

    //~ debug infos
    fps_stats fps_stats_{};
};

#pragma region ENGINE_MAIN
engine::engine()
: impl_(std::make_unique<engine::implementation>())
{}

engine::~engine()
{
    impl_->deinitialize();
}


bool engine::initialize() const
{
    return impl_->initialize();
}

void engine::update()
{
    impl_->update();
}

bool engine::should_close() const noexcept
{
    return impl_->should_close();
}

float engine::delta_time() const noexcept
{
    return impl_->delta_time();
}

fps_stats engine::get_fps_stats() const noexcept
{
    return impl_->get_fps_stats();
}

void engine::set_target_fps(const std::uint32_t target_fps) const
{
    impl_->set_target_fps(target_fps);
}
#pragma endregion //~ Engine Main

#pragma region ENGINE_IMPLEMENTATION

engine::implementation::~implementation()
{
    deinitialize();
}

bool engine::implementation::initialize()
{
    initialize_subsystems();

    if (not configure_subsystems()) return false;
    configure_tickables  ();

    //~ reset timer states
    timer_->reset();
    timer_->set_target_frame_ps(config::DEFAULT_ENGINE_FPS);

    return true;
}

void engine::implementation::deinitialize()
{
    //~ deinitialize all subsystems
    for (const auto system: std::views::reverse(subsystem_))
    {
        if (system)
        {
            system->deinitialize();
        }
    }
}

void engine::implementation::update()
{
    timer_->step();

    update_tickables();
    compute_fps     ();
}

bool engine::implementation::should_close() const
{
    return windows_->should_close();
}

float engine::implementation::delta_time() const
{
    return timer_->delta_time();
}

fps_stats engine::implementation::get_fps_stats() const noexcept
{
    return fps_stats_;
}

void engine::implementation::set_target_fps(const std::uint32_t target_fps) const
{
    timer_->set_target_frame_ps(target_fps);
}

void engine::implementation::initialize_subsystems()
{
    timer_      = feature::locator::resolve<utils::timer>();
    windows_    = feature::locator::resolve<platform::windows>();

    //~ setup subsystems
    windows_->setup_config(reinterpret_cast<const std::byte*>(&config_manager_.windows_config()));
}

bool engine::implementation::configure_subsystems()
{
    //~ register subsystems
    REGISTER_FEATURE_TO_SCHEDULER(subsystem_, audio::system);
    subsystem_.register_type(std::ref(windows_));

    render_  = feature::locator::resolve<graphics::render>();
    subsystem_.register_type(std::ref(render_));

    //~ TODO: Add Physics Subsystem - should depend on renderer

    //~ configure dependencies
    subsystem_.add_dependency(render_, windows_);

    //~ initialize systems
    for (auto* system: subsystem_)
    {
        if (not system || !system->initialize())
        {
            //~ TODO: Add Logging
            return false;
        }
    }
    return true;
}

void engine::implementation::configure_tickables()
{
    //~ register
    REGISTER_FEATURE_TO_SCHEDULER(tickables_, audio::system);
    REGISTER_FEATURE_TO_SCHEDULER(tickables_, events::dispatcher);

    tickables_.register_type(std::ref(windows_));
    tickables_.register_type(std::ref(render_));

    //~ configure dependencies
    tickables_.add_dependency(render_,  windows_);
}

void engine::implementation::update_tickables()
{
    const float dt = timer_->delta_time();

    //~ begin from parent to child
    for (const auto tickable: tickables_)
        if (tickable) tickable->begin_update(dt);

    //~ end from child to parent
    for (const auto iter : std::views::reverse(tickables_))
        iter->end_update();

}
void engine::implementation::compute_fps()
{
    static auto window_start = utils::timer::now();
    static std::uint32_t mt_frames = 0;
    ++mt_frames;

    const auto  now = utils::timer::now();
    const float elapsed = std::chrono::duration<float>(now - window_start).count();

    if (elapsed >= 0.5f)
    {
        fps_stats_.main_thread   = static_cast<float>(mt_frames) / elapsed;
        fps_stats_.render_thread = render_->fps();

        window_start = now;
        mt_frames    = 0;

#if COTS_DEBUG || COTS_RELEASE //~ fps displays on title bar
        display_fps();
#endif

        window_start = now;
        mt_frames    = 0;
    }
}

void engine::implementation::display_fps()
{
    windows_->set_debug(std::format(
        "MT {} fps | RT {} fps",
        fps_stats_.main_thread, fps_stats_.render_thread)
    );
}

#pragma endregion // Engine Implementation
