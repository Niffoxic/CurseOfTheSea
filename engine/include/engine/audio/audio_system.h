// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_system_H
#define CURSEOFTHESEA_system_H

#include <memory>
#include "engine/core/framework/interface/subsystem.h"
#include "engine/core/framework/interface/tickable.h"
#include "engine/core/framework/interface/audio_backend.h"

namespace cots::audio
{
    class system final: public interface::subsystem, public interface::tickable
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
        bool load_sound  (audio::sound_id id, std::string_view path) const;
        void unload_sound(audio::sound_id id) const;

        //~ playback shortcuts
        audio::handle play_one_shot(audio::sound_id id, float volume = 1.f) const;
        audio::handle play_loop    (audio::sound_id id, audio::bus target = bus::sfx) const;

        void stop       (audio::handle handle, float fade_ms = 0.f) const;
        void set_volume (audio::handle handle, float volume) const;
        void set_pitch  (audio::handle handle, float pitch)  const;

        //~ bus
        void set_bus_volume(audio::bus b, float v) const;
        void set_bus_muted (audio::bus b, bool muted) const;

        //~ global
        void pause_all () const;
        void resume_all() const;

        interface::audio_backend* backend() const noexcept;
    private:
        std::unique_ptr<interface::audio_backend> backend_{ nullptr };
    };
}

#endif //CURSEOFTHESEA_system_H
