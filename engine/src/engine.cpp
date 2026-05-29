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

using namespace trishul;

struct engine::impl
{
    //~ initialization
    void initialize_features();
    void regulate_subsystems();
    void regulate_tickable  ();

    //~ per frame
    void update_tickable();
    void compute_fps    ();

    //~ members
    dependency_handler<interfaces::subsystems> subsystem_scheduler_;
    dependency_handler<interfaces::tickable>   tickable_scheduler_;
    fps_information fps_{};
};

#pragma region ENGINE

engine::engine()
: p_(std::make_unique<impl>())
{}

engine::~engine()
{
    if (not p_) return;
    for (const auto subsystem : std::views::reverse(p_->subsystem_scheduler_))
    {
        if (subsystem) subsystem->deinitialize();
    }
}

bool engine::initialize() const
{
    ENGINE_ASSERT_MSG(p_, "Corrupted Engine or destroyed already");
    p_->initialize_features();
    p_->regulate_subsystems();
    p_->regulate_tickable  ();
    return false;
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
    return true;
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

void engine::impl::initialize_features()
{

}

void engine::impl::regulate_subsystems()
{

}

void engine::impl::regulate_tickable()
{

}

void engine::impl::update_tickable()
{

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

        window_start = now;
        mt_frames    = 0;
    }
}

#pragma endregion
