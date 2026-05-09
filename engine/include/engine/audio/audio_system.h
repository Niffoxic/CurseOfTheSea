// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_system_H
#define CURSEOFTHESEA_system_H

#include <memory>
#include "engine/core/framework/interface/subsystem.h"
#include "engine/core/framework/interface/tickable.h"
#include "engine/core/framework/interface/audio_backend.h"

namespace cots::audio
{
    struct play_3d_params
    {
        float      pos[3]      { 0.f, 0.f, 0.f };
        float      min_distance{ 1.f };
        float      max_distance{ 30.f };
        audio::bus bus         { bus::sfx };
        bool       looping     { true };
    };

    class system final: public interfaces::subsystem, public interfaces::tickable
    {
    public:
         system() = default;
        ~system() override = default;

        //~ non copiable or movable
        system(const system&) = delete;
        system(system&&)      = delete;

        system& operator=(const system&) = delete;
        system& operator=(system&&)      = delete;

        [[nodiscard]]
        bool initialize  ()          override;
        void deinitialize() noexcept override;

        void begin_update(float dt)  override;
        void end_update  ()          override;

        //~ resource management
        [[nodiscard]]
        bool load_sound  (audio::sound_id id, std::string_view path, bool positional) const;
        void unload_sound(audio::sound_id id) const;

        //~ playback shortcuts
        [[nodiscard]] audio::handle play_one_shot(audio::sound_id id, float volume = 1.f)           const;
        [[nodiscard]] audio::handle play_loop    (audio::sound_id id, audio::bus target = bus::sfx) const;

        void stop       (audio::handle handle, float fade_ms = 0.f) const;
        void set_volume (audio::handle handle, float volume) const;
        void set_pitch  (audio::handle handle, float pitch)  const;

        //~ 3d
        [[nodiscard]] audio::handle play_3d(audio::sound_id id, const play_3d_params& param) const;

        void set_position(audio::handle h, const float pos[3], const float vel[3]) const;
        void set_listener(const interfaces::listener_state& s) const;

        //~ bus
        void set_bus_volume(audio::bus b, float v) const;
        void set_bus_muted (audio::bus b, bool muted) const;

        //~ global
        void pause_all () const;
        void resume_all() const;

        [[nodiscard]] interfaces::audio_backend* backend() const noexcept;
    private:
        std::unique_ptr<interfaces::audio_backend> backend_{ nullptr };
    };
}

#endif //CURSEOFTHESEA_system_H
