// Created by Niffoxic (Harsh Dubey)
#include "engine/system/service_registry.h"
#include "engine/system/feature_locator.h"
#include "engine/audio/audio_system.h"

namespace
{
    using cots::module::audio_handle;

    audio_handle to_module(cots::audio::handle h) noexcept
    {
        return { h.index, h.generation };
    }

    cots::audio::handle from_module(audio_handle h) noexcept
    {
        return { h.index, h.generation };
    }

    bool load_sound(const char* path, const bool positional)
    {
        const auto sys = cots::feature::locator::resolve<cots::audio::system>();
        const auto id  = cots::audio::hash_sound_id(path);
        return sys->load_sound(id, path, positional);
    }

    void unload_sound(const char* path)
    {
        const auto sys = cots::feature::locator::resolve<cots::audio::system>();
        const auto id  = cots::audio::hash_sound_id(path);
        sys->unload_sound(id);
    }

    audio_handle play_oneshot(const char* path, const float volume)
    {
        const auto sys = cots::feature::locator::resolve<cots::audio::system>();
        const auto id  = cots::audio::hash_sound_id(path);
        return to_module(sys->play_one_shot(id, volume));
    }

    audio_handle play_3d(const char* path, const float pos[3],
                         const float min_distance, const float max_distance)
    {
        const auto sys = cots::feature::locator::resolve<cots::audio::system>();
        const auto id  = cots::audio::hash_sound_id(path);

        cots::audio::play_3d_params p{};
        p.pos[0]       = pos[0];
        p.pos[1]       = pos[1];
        p.pos[2]       = pos[2];
        p.min_distance = min_distance;
        p.max_distance = max_distance;
        p.looping      = false;
        return to_module(sys->play_3d(id, p));
    }

    void stop(audio_handle h, const float fade_ms)
    {
        const auto sys = cots::feature::locator::resolve<cots::audio::system>();
        sys->stop(from_module(h), fade_ms);
    }

    void set_volume(audio_handle h, const float volume)
    {
        const auto sys = cots::feature::locator::resolve<cots::audio::system>();
        sys->set_volume(from_module(h), volume);
    }

    void set_position(audio_handle h, const float pos[3], const float vel[3])
    {
        const auto sys = cots::feature::locator::resolve<cots::audio::system>();
        sys->set_position(from_module(h), pos, vel);
    }

    void set_listener(const float pos[3], const float fwd[3], const float up[3])
    {
        const auto sys = cots::feature::locator::resolve<cots::audio::system>();

        cots::interface::listener_state s{};
        s.position[0] = pos[0]; s.position[1] = pos[1]; s.position[2] = pos[2];
        s.forward [0] = fwd[0]; s.forward [1] = fwd[1]; s.forward [2] = fwd[2];
        s.up      [0] = up [0]; s.up      [1] = up [1]; s.up      [2] = up [2];
        sys->set_listener(s);
    }

    void set_bus_volume(const int bus_index, const float volume)
    {
        const auto sys = cots::feature::locator::resolve<cots::audio::system>();
        sys->set_bus_volume(static_cast<cots::audio::bus>(bus_index), volume);
    }

    void pause_all ()
    {
        cots::feature::locator::resolve<cots::audio::system>()->pause_all();
    }

    void resume_all()
    {
        cots::feature::locator::resolve<cots::audio::system>()->resume_all();
    }

    void install(cots::module::services& s)
    {
        s.audio.load_sound      = &load_sound;
        s.audio.unload_sound    = &unload_sound;
        s.audio.play_oneshot    = &play_oneshot;
        s.audio.play_3d         = &play_3d;
        s.audio.stop            = &stop;
        s.audio.set_volume      = &set_volume;
        s.audio.set_position    = &set_position;
        s.audio.set_listener    = &set_listener;
        s.audio.set_bus_volume  = &set_bus_volume;
        s.audio.pause_all       = &pause_all;
        s.audio.resume_all      = &resume_all;
    }
}

COTS_INSTALL_SERVICES(install)
