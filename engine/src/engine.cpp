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

#include "trishul/services.h"

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
    void update_tickable();
    void compute_fps    ();

    //~ members
    engine_create_info create_info_;
    dependency_handler<interfaces::subsystems> subsystem_scheduler_;
    dependency_handler<interfaces::tickable>   tickable_scheduler_;
    fps_information fps_{};

    //~ services
    platform_window* window_ = nullptr;
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

    logger::instance().deinitialize();
}

bool engine::initialize() const
{
    ENGINE_ASSERT_MSG(p_, "Corrupted Engine or destroyed already");
    LOG_INFO("engine initialize begin");

    p_->initialize_services();

    if (not p_->regulate_services())
    {
        LOG_CRITICAL("engine initialize failed during service regulation");
        return false;
    }

    p_->regulate_tickable();
    LOG_INFO("engine initialize complete");
    return true;
}

void engine::tick() const
{
    ENGINE_ASSERT_MSG(p_, "Corrupted Engine or destroyed already");
    p_->compute_fps    ();
    p_->update_tickable();
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
    return 0.f;
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
    LOG_DEBUG("acquiring platform window service");
    window_ = service_locator::get<platform_window>();

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
}

bool engine::impl::regulate_services()
{
    //~ register and build dependencies
    LOG_DEBUG("registering subsystems");
    subsystem_scheduler_.register_type(window_);

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

void engine::impl::regulate_tickable()
{
    //~ register and build dependencies
    LOG_DEBUG("registering tickables");
    tickable_scheduler_.register_type(window_);
}

void engine::impl::update_tickable()
{
    //~ update begin
    for (const auto tickable: tickable_scheduler_)
    {
        if (tickable) tickable->begin_update(0.f);
    }

    //~ update end
    for (const auto iter : std::views::reverse(tickable_scheduler_))
    {
        if (iter) iter->end_update();
    }
}

void engine::impl::compute_fps()
{
    using clock = std::chrono::steady_clock;

    static auto window_start = clock::now();
    static int  mt_frames    = 0;
    ++mt_frames;

    const auto  now = clock::now();
    const float elapsed = std::chrono::duration<float>(now - window_start).count();

    if (elapsed >= 0.5f)
    {
        fps_.main_thread     = static_cast<float>(mt_frames) / elapsed;
        fps_.main_thread_ms  = (elapsed * 1000.f) / static_cast<float>(mt_frames);

        LOG_TRACE("fps {} {:.3f} ms", fps_.main_thread, fps_.main_thread_ms);

        window_start = now;
        mt_frames    = 0;
    }
}

#pragma endregion
