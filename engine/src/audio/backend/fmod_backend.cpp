// Created by Niffoxic (Harsh Dubey)
#include "engine/audio/backend/fmod_backend.h"

#include <fmod.hpp>
#include <fmod_errors.h>
#include <ranges>
#include <spdlog/spdlog.h>

namespace
{
    bool fmod_check(const FMOD_RESULT r, const char* where)
    {
        if (r == FMOD_OK) return true;
        spdlog::error("[audio:fmod] {} failed: {}", where, FMOD_ErrorString(r));
        return false;
    }
}

cots::audio::backend::fmod_backend:: fmod_backend() = default;
cots::audio::backend::fmod_backend::~fmod_backend() = default;

bool cots::audio::backend::fmod_backend::initialize()
{
    if (!fmod_check(FMOD::System_Create(&system_), "System_Create"))
        return false;

    if (!fmod_check(system_->init(
        max_voices,
        FMOD_INIT_3D_RIGHTHANDED,
        nullptr), "System::init"))
        return false;

    system_->set3DSettings(
        1.0f,   // doppler scale
        1.0f,   // distance factor: 1 unit = 1 meter
        1.0f);  // rolloff scale

    // create channel groups for buses
    FMOD::ChannelGroup* master = nullptr;
    if (!fmod_check(system_->getMasterChannelGroup(&master), "getMasterChannelGroup"))
        return false;
    groups_[static_cast<std::size_t>(bus::master)] = master;

    constexpr const char* names[] = { nullptr, "sfx", "music", "ui", "voice" };
    for (std::size_t i = 1; i < static_cast<std::size_t>(bus::count); ++i)
    {
        FMOD::ChannelGroup* group = nullptr;
        if (!fmod_check(system_->createChannelGroup(names[i], &group), "createChannelGroup"))
            return false;
        if (!fmod_check(master->addGroup(group), "master->addGroup"))
            return false;
        groups_[i] = group;
    }

    spdlog::info("[audio:fmod] initialized ({} voices)", max_voices);
    return true;
}

void cots::audio::backend::fmod_backend::deinitialize() noexcept
{
    if (!system_) return;

    for (const auto &sound: sounds_ | std::views::values)
        if (sound) sound->release();
    sounds_.clear();

    groups_.fill(nullptr);

    system_->close();
    system_->release();
    system_ = nullptr;
}

void cots::audio::backend::fmod_backend::begin_update(float dt)
{
    if (system_) system_->update();
}

void cots::audio::backend::fmod_backend::end_update()
{

}

bool cots::audio::backend::fmod_backend::load_sound(
    const audio::sound_id id,
    const std::string_view path,
    const bool positional)
{
    if (sounds_.contains(id.value))
    {
        spdlog::warn("[audio:fmod] sound already loaded: {}", path);
        return true;
    }

    const std::string null_terminated{ path };

    const FMOD_MODE mode = positional ? (FMOD_3D | FMOD_3D_LINEARROLLOFF) : FMOD_2D;

    FMOD::Sound* sound = nullptr;
    const auto r = system_->createSound(
        null_terminated.c_str(),
        mode,
        nullptr,
        &sound);

    if (!fmod_check(r, "createSound")) return false;

    sounds_[id.value] = sound;
    spdlog::debug("[audio:fmod] loaded: {} ({})", path, positional ? "3D" : "2D");
    return true;
}

void cots::audio::backend::fmod_backend::unload_sound(audio::sound_id id)
{
    const auto it = sounds_.find(id.value);
    if (it == sounds_.end()) return;

    if (it->second) it->second->release();
    sounds_.erase(it);
}

cots::audio::handle cots::audio::backend::fmod_backend::play(
    const audio::sound_id id,
    const interface::play_params &params)
{
    const auto it = sounds_.find(id.value);
    if (it == sounds_.end())
    {
        spdlog::warn("[audio:fmod] play: unknown sound id {}", id.value);
        return audio::handle::invalid();
    }

    // configure looping
    it->second->setMode(params.looping ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

    FMOD::Channel* channel = nullptr;
    if (!fmod_check(system_->playSound(it->second,
        group_for(params.bus),
        true, &channel),
                    "playSound"))
        return audio::handle::invalid();

    channel->setVolume(params.volume);
    channel->setPitch (params.pitch);
    channel->setPaused(false);

    if (params.positional)
    {
        FMOD_MODE sound_mode = 0;
        it->second->getMode(&sound_mode);

        if (sound_mode & FMOD_3D)
        {
            const FMOD_VECTOR pos{ params.position[0], params.position[1], params.position[2] };
            const FMOD_VECTOR vel{ params.velocity[0], params.velocity[1], params.velocity[2] };
            channel->set3DAttributes(&pos, &vel);
            channel->set3DMinMaxDistance(params.min_distance, params.max_distance);
        }
        else
        {
            spdlog::warn("[audio:fmod] play: positional=true but sound loaded as 2D (id={})", id.value);
        }
    }

    const auto idx = acquire_slot();
    if (idx == 0)
    {
        channel->stop();
        spdlog::warn("[audio:fmod] no free voice slots");
        return audio::handle::invalid();
    }

    const std::uint32_t gen = next_generation_++;
    if (next_generation_ == 0) next_generation_ = 1;

    slots_[idx] = slot{ channel, gen };
    return audio::handle{ idx, gen };
}

void cots::audio::backend::fmod_backend::stop(
    const audio::handle handle,
    const float fade_ms)
{
    FMOD::Channel* ch = resolve(handle);
    if (!ch) return;

    if (fade_ms <= 0.0f)
    {
        ch->stop();
        return;
    }

    unsigned long long parent_clock = 0;
    int rate = 48000;
    system_->getSoftwareFormat(
        &rate,
        nullptr,
        nullptr
    );
    ch->getDSPClock(nullptr, &parent_clock);

    const auto fade_samples = static_cast<unsigned long long>(
        (fade_ms / 1000.0f) * static_cast<float>(rate));

    ch->addFadePoint(parent_clock, 1.0f);
    ch->addFadePoint(parent_clock + fade_samples, 0.0f);
    ch->setDelay(0, parent_clock + fade_samples, true);  // stop after fade
}

void cots::audio::backend::fmod_backend::set_volume(
    const audio::handle handle,
    const float volume)
{
    if (auto* ch = resolve(handle)) ch->setVolume(volume);
}

void cots::audio::backend::fmod_backend::set_pitch(
    const audio::handle handle,
    const float pitch)
{
    if (auto* ch = resolve(handle)) ch->setPitch(pitch);
}

bool cots::audio::backend::fmod_backend::is_playing(const audio::handle handle) const
{
    FMOD::Channel* ch = resolve(handle);
    if (!ch) return false;

    bool playing = false;
    ch->isPlaying(&playing);
    return playing;
}

void cots::audio::backend::fmod_backend::set_bus_volume(const audio::bus b, const float v)
{
    if (auto* g = group_for(b)) g->setVolume(v);
}

void cots::audio::backend::fmod_backend::set_bus_muted(const audio::bus b, const bool muted)
{
    if (auto* g = group_for(b)) g->setMute(muted);
}

void cots::audio::backend::fmod_backend::set_position(
    audio::handle handle,
    const float pos[3],
    const float vel[3])
{
    auto* ch = resolve(handle);
    if (!ch) return;

    const FMOD_VECTOR p{ pos[0], pos[1], pos[2] };
    const FMOD_VECTOR v{ vel[0], vel[1], vel[2] };
    ch->set3DAttributes(&p, &v);
}

void cots::audio::backend::fmod_backend::set_listener(
    const interface::listener_state &s)
{
    if (!system_) return;

    const FMOD_VECTOR pos{ s.position[0], s.position[1], s.position[2] };
    const FMOD_VECTOR vel{ s.velocity[0], s.velocity[1], s.velocity[2] };
    const FMOD_VECTOR fwd{ s.forward[0],  s.forward[1],  s.forward[2]  };
    const FMOD_VECTOR up { s.up[0],       s.up[1],       s.up[2]       };

    system_->set3DListenerAttributes(0, &pos, &vel, &fwd, &up);
}

void cots::audio::backend::fmod_backend::pause_all()
{
    if (auto* m = groups_[static_cast<std::size_t>(bus::master)])
        m->setPaused(true);
}

void cots::audio::backend::fmod_backend::resume_all()
{
    if (auto* m = groups_[static_cast<std::size_t>(bus::master)])
        m->setPaused(false);
}

FMOD::Channel * cots::audio::backend::fmod_backend::resolve(const audio::handle h) const
{
    if (!h.valid() || h.index >= max_voices) return nullptr;

    const auto&[channel, generation] = slots_[h.index];
    if (generation != h.generation) return nullptr;
    if (!channel) return nullptr;

    bool playing = false;
    if (channel->isPlaying(&playing) != FMOD_OK || !playing) return nullptr;

    return channel;
}

std::uint32_t cots::audio::backend::fmod_backend::acquire_slot()
{
    for (std::uint32_t i = 1; i < max_voices; ++i)
    {
        auto&[channel, generation] = slots_[i];
        if (!channel)
        {
            return i;
        }

        bool playing = false;
        if (channel->isPlaying(&playing) != FMOD_OK || !playing)
        {
            channel    = nullptr;
            generation = 0;
            return i;
        }
    }
    return 0;
}

FMOD::ChannelGroup* cots::audio::backend::fmod_backend::group_for(const bus b) const noexcept
{
    const auto i = static_cast<std::size_t>(b);
    if (i >= groups_.size()) return groups_[0];
    return groups_[i];
}
