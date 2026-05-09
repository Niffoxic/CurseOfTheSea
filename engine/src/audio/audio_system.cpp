// Created by Niffoxic (Harsh Dubey)
#include "engine/audio/audio_system.h"
#include "engine/audio/backend/null_backend.h"
#include "engine/audio/backend/fmod_backend.h"

#include <spdlog/spdlog.h>

#include "engine/core/cots_assert.h"

bool cots::audio::system::initialize()
{
    backend_ = std::make_unique<backend::fmod_backend>();

    if (not backend_->initialize())
    {
        spdlog::error("Failed to initialize audio backend");
        return false;
    }
    return true;
}

void cots::audio::system::deinitialize() noexcept
{
    if (backend_) backend_->deinitialize();
    backend_.reset();
}

void cots::audio::system::begin_update(float dt)
{
    if (backend_) backend_->begin_update(dt);
}

void cots::audio::system::end_update()
{
    if (backend_) backend_->end_update();
}

bool cots::audio::system::load_sound(
    const audio::sound_id id,
    const std::string_view path,
    const bool positional) const
{
    COTS_ASSERT_MSG(backend_, "Audio backend not initialized");
    return backend_->load_sound(id, path, positional);
}

void cots::audio::system::unload_sound(const audio::sound_id id) const
{
    COTS_ASSERT_MSG(backend_, "Audio backend not initialized");
    backend_->unload_sound(id);
}

cots::audio::handle cots::audio::system::play_one_shot(
    const audio::sound_id id,
    const float volume) const
{
    COTS_ASSERT_MSG(backend_, "Audio backend not initialized");

    interfaces::play_params params{};
    params.volume = volume;
    params.looping = false;
    return backend_->play(id, params);
}

cots::audio::handle cots::audio::system::play_loop(
    const audio::sound_id id,
    const audio::bus target) const
{
    COTS_ASSERT_MSG(backend_, "Audio backend not initialized");
    interfaces::play_params params{};
    params.bus     = target;
    params.looping = true;
    return backend_->play(id, params);
}

void cots::audio::system::stop(
    const audio::handle handle,
    const float fade_ms) const
{
    COTS_ASSERT_MSG(backend_, "Audio backend not initialized");
    backend_->stop(handle, fade_ms);
}

void cots::audio::system::set_volume(
    const audio::handle handle,
    const float volume) const
{
    COTS_ASSERT_MSG(backend_, "Audio backend not initialized");
    backend_->set_volume(handle, volume);
}

void cots::audio::system::set_pitch(
    const audio::handle handle,
    const float pitch) const
{
    COTS_ASSERT_MSG(backend_, "Audio backend not initialized");
    backend_->set_pitch(handle, pitch);
}

cots::audio::handle cots::audio::system::play_3d(
    const audio::sound_id id,
    const play_3d_params& param) const
{
    COTS_ASSERT_MSG(backend_, "Audio backend not initialized");

    interfaces::play_params params{};
    params.bus          = param.bus;
    params.looping      = param.looping;
    params.positional   = true;
    params.position[0]  = param.pos[0];
    params.position[1]  = param.pos[1];
    params.position[2]  = param.pos[2];
    params.min_distance = param.min_distance;
    params.max_distance = param.max_distance;
    return backend_->play(id, params);
}

void cots::audio::system::set_position(
    const audio::handle h,
    const float pos[3],
    const float vel[3]) const
{
    COTS_ASSERT_MSG(backend_, "Audio backend not initialized");
    backend_->set_position(h, pos, vel);
}

void cots::audio::system::set_listener(const interfaces::listener_state &s) const
{
    COTS_ASSERT_MSG(backend_, "Audio backend not initialized");
    backend_->set_listener(s);
}

void cots::audio::system::set_bus_volume(const audio::bus b, const float v) const
{
    COTS_ASSERT_MSG(backend_, "Audio backend not initialized");
    backend_->set_bus_volume(b, v);
}

void cots::audio::system::set_bus_muted(const audio::bus b, const bool muted) const
{
    COTS_ASSERT_MSG(backend_, "Audio backend not initialized");
    backend_->set_bus_muted(b, muted);
}

void cots::audio::system::pause_all() const
{
    COTS_ASSERT_MSG(backend_, "Audio backend not initialized");
    backend_->pause_all();
}

void cots::audio::system::resume_all() const
{
    COTS_ASSERT_MSG(backend_, "Audio backend not initialized");
    backend_->resume_all();
}

cots::interfaces::audio_backend * cots::audio::system::backend() const noexcept
{
    return backend_.get();
}
