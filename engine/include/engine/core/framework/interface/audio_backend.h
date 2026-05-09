// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_AUDIO_BACKEND_H
#define CURSEOFTHESEA_AUDIO_BACKEND_H

#include <string_view>

#include "subsystem.h"
#include "tickable.h"
#include "engine/audio/audio_handle.h"
#include "engine/audio/audio_bus.h"
#include "engine/audio/sound_id.h"

namespace cots::interface
{
    //~ following Y Up right hand convention
    struct play_params
    {
        float      volume { 1.f };
        float      pitch  { 1.f };
        audio::bus bus    { audio::bus::master };
        bool       looping{ false };

        bool       positional   { false };
        float      position[3]  { 0.f, 0.f, 0.f };
        float      velocity[3]  { 0.f, 0.f, 0.f };
        float      min_distance { 1.f };
        float      max_distance { 100.f };
    };

    struct listener_state
    {
        float position[3] { 0, 0, 0 };
        float velocity[3] { 0, 0, 0 };
        float forward[3]  { 0, 0, -1 };
        float up[3]       { 0, 1, 0 };
    };

    class audio_backend: public subsystem, public tickable
    {
    public:
        //~ resource management
        [[nodiscard]]
        virtual bool load_sound  (audio::sound_id id, std::string_view path, bool positional) = 0;
        virtual void unload_sound(audio::sound_id id)                         = 0;

        //~ playback
        [[nodiscard]]
        virtual audio::handle play      (audio::sound_id id, const play_params& params) = 0;
        virtual void          stop      (audio::handle handle, float fade_ms)           = 0;
        virtual void          set_volume(audio::handle handle, float volume)            = 0;
        virtual void          set_pitch (audio::handle handle, float pitch)             = 0;

        [[nodiscard]]
        virtual bool is_playing(audio::handle handle) const = 0;

        //~ 3d
        virtual void set_position(audio::handle handle,
                                  const float pos[3],
                                  const float vel[3])               = 0;
        virtual void set_listener(const listener_state& s)          = 0;

        // bus control
        virtual void set_bus_volume(audio::bus b, float v)    = 0;
        virtual void set_bus_muted (audio::bus b, bool muted) = 0;

        // global
        virtual void pause_all () = 0;
        virtual void resume_all() = 0;
    };
} // namespace cots::interface

#endif //CURSEOFTHESEA_AUDIO_BACKEND_H
