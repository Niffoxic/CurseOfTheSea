// Created by Niffoxic (Harsh Dubey)
#include "engine/audio/backend/null_backend.h"
#include <spdlog/spdlog.h>

bool cots::audio::backend::null_backend::initialize()
{
    spdlog::info("Audio backend: Null");
    return true;
}

void cots::audio::backend::null_backend::deinitialize() noexcept
{
    spdlog::info("Audio backend: Null deinitialized");
}

void cots::audio::backend::null_backend::begin_update(float dt)
{

}

void cots::audio::backend::null_backend::end_update()
{

}

bool cots::audio::backend::null_backend::load_sound(
    const audio::sound_id id,
    const std::string_view path,
    const bool positional)
{
    spdlog::info("null loaded sound: {}", path);
    return true;
}

void cots::audio::backend::null_backend::unload_sound(audio::sound_id id)
{

}

cots::audio::handle cots::audio::backend::null_backend::play(
    audio::sound_id id,
    const interfaces::play_params &params)
{
    return audio::handle{next_index_++, next_generation_++};
}

void cots::audio::backend::null_backend::stop(audio::handle handle, float fade_md)
{}

void cots::audio::backend::null_backend::set_volume(audio::handle handle, float volume)
{}

void cots::audio::backend::null_backend::set_pitch(audio::handle handle, float pitch)
{}

bool cots::audio::backend::null_backend::is_playing(audio::handle handle) const
{
    return false;
}

void cots::audio::backend::null_backend::set_bus_volume(audio::bus b, float v)
{}

void cots::audio::backend::null_backend::set_bus_muted(audio::bus b, bool muted)
{}

void cots::audio::backend::null_backend::pause_all()
{}

void cots::audio::backend::null_backend::resume_all()
{}

void cots::audio::backend::null_backend::set_position(
    audio::handle handle,
    const float pos[3], const float vel[3])
{
    spdlog::info("null set position: {}, {}, {}", pos[0], pos[1], pos[2]);
}

void cots::audio::backend::null_backend::set_listener(
    const interfaces::listener_state &s)
{
    spdlog::info("null lisenters");
}
