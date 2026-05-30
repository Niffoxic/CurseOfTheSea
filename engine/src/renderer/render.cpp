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
#include "trishul/core/engine_assert.h"
#include "trishul/core/engine_config.h"

#include <d3d12.h>
#include <thread>
#include <mutex>
#include <array>

#include "trishul/utils/logger.h"

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
    void on_window_resized(const events::window_resized& event);

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
    //~ TODO: gotta add devices and all

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
    //~ TODO: Initialize D3D12 driver
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

}

void graphics::impl::subscribe_events()
{

}

void graphics::impl::unsubscribe_events()
{

}

void graphics::impl::on_window_resized(const events::window_resized &event)
{

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
