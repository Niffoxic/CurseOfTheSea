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

    auto sys() { return cots::feature::locator::resolve<cots::audio::system>(); }

    //~ resource
    bool load_sound(const char* path, const bool positional)
    {
        const auto id = cots::audio::hash_sound_id(path);
        return sys()->load_sound(id, path, positional);
    }

    void unload_sound(const char* path)
    {
        const auto id = cots::audio::hash_sound_id(path);
        sys()->unload_sound(id);
    }

    //~ 2D playback
    audio_handle play_oneshot(const char* path, const float volume)
    {
        const auto id = cots::audio::hash_sound_id(path);
        return to_module(sys()->play_one_shot(id, volume));
    }

    audio_handle play_loop(const char* path, const int bus_index, const float volume)
    {
        const auto id     = cots::audio::hash_sound_id(path);
        const auto bus    = static_cast<cots::audio::bus>(bus_index);
        const auto handle = sys()->play_loop(id, bus);

        if (handle.valid()) sys()->set_volume(handle, volume);
        return to_module(handle);
    }

    //~ 3D playback
    audio_handle play_3d(const char* path, const float pos[3],
                         const float min_distance, const float max_distance)
    {
        const auto id = cots::audio::hash_sound_id(path);

        cots::audio::play_3d_params p{};
        p.pos[0]       = pos[0];
        p.pos[1]       = pos[1];
        p.pos[2]       = pos[2];
        p.min_distance = min_distance;
        p.max_distance = max_distance;
        p.looping      = false;
        return to_module(sys()->play_3d(id, p));
    }

    audio_handle play_3d_loop(const char* path, const float pos[3],
                              const float min_distance, const float max_distance,
                              const float volume)
    {
        const auto id = cots::audio::hash_sound_id(path);

        cots::audio::play_3d_params p{};
        p.pos[0]       = pos[0];
        p.pos[1]       = pos[1];
        p.pos[2]       = pos[2];
        p.min_distance = min_distance;
        p.max_distance = max_distance;
        p.looping      = true;

        const auto handle = sys()->play_3d(id, p);
        if (handle.valid()) sys()->set_volume(handle, volume);
        return to_module(handle);
    }

    //~ control
    void stop(audio_handle h, const float fade_ms)
    {
        sys()->stop(from_module(h), fade_ms);
    }

    void set_volume(audio_handle h, const float volume)
    {
        sys()->set_volume(from_module(h), volume);
    }

    void set_pitch(audio_handle h, const float pitch)
    {
        sys()->set_pitch(from_module(h), pitch);
    }

    bool is_playing(audio_handle h)
    {
        return sys()->backend()->is_playing(from_module(h));
    }

    //~ 3D
    void set_position(audio_handle h, const float pos[3], const float vel[3])
    {
        sys()->set_position(from_module(h), pos, vel);
    }

    void set_listener(const float pos[3], const float fwd[3], const float up[3])
    {
        cots::interfaces::listener_state s{};
        s.position[0] = pos[0]; s.position[1] = pos[1]; s.position[2] = pos[2];
        s.forward [0] = fwd[0]; s.forward [1] = fwd[1]; s.forward [2] = fwd[2];
        s.up      [0] = up [0]; s.up      [1] = up [1]; s.up      [2] = up [2];
        sys()->set_listener(s);
    }

    //~ bus & global
    void set_bus_volume(const int bus_index, const float volume)
    {
        sys()->set_bus_volume(static_cast<cots::audio::bus>(bus_index), volume);
    }

    void set_bus_muted(const int bus_index, const bool muted)
    {
        sys()->set_bus_muted(static_cast<cots::audio::bus>(bus_index), muted);
    }

    void pause_all () { sys()->pause_all(); }
    void resume_all() { sys()->resume_all(); }

    void install(cots::module::services& s)
    {
        //~ resource
        s.audio.load_sound     = &load_sound;
        s.audio.unload_sound   = &unload_sound;

        //~ 2D
        s.audio.play_oneshot   = &play_oneshot;
        s.audio.play_loop      = &play_loop;

        //~ 3D
        s.audio.play_3d        = &play_3d;
        s.audio.play_3d_loop   = &play_3d_loop;

        //~ control
        s.audio.stop           = &stop;
        s.audio.set_volume     = &set_volume;
        s.audio.set_pitch      = &set_pitch;
        s.audio.is_playing     = &is_playing;

        //~ 3D
        s.audio.set_position   = &set_position;
        s.audio.set_listener   = &set_listener;

        //~ bus & global
        s.audio.set_bus_volume = &set_bus_volume;
        s.audio.set_bus_muted  = &set_bus_muted;
        s.audio.pause_all      = &pause_all;
        s.audio.resume_all     = &resume_all;
    }
}

COTS_INSTALL_SERVICES(install)