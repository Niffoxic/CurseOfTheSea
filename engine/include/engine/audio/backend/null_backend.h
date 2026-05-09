// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_NULL_BACKEND_H
#define CURSEOFTHESEA_NULL_BACKEND_H

#include "engine/core/framework/interface/audio_backend.h"

namespace cots::audio::backend
{
    class null_backend final: public interfaces::audio_backend
    {
    public:
        [[nodiscard]]
        bool initialize  ()          override;
        void deinitialize() noexcept override;

        void begin_update(float dt) override;
        void end_update  ()         override;

        [[nodiscard]]
        bool load_sound  (audio::sound_id id, std::string_view path, bool positional) override;
        void unload_sound(audio::sound_id id)                        override;

        [[nodiscard]]
        audio::handle play      (audio::sound_id id, const interfaces::play_params &params)  override;
        void          stop      (audio::handle handle, float fade_md)                       override;
        void          set_volume(audio::handle handle, float volume)                        override;
        void          set_pitch (audio::handle handle, float pitch)                         override;

        [[nodiscard]]
        bool is_playing(audio::handle handle) const override;

        void set_bus_volume(audio::bus b, float v)    override;
        void set_bus_muted (audio::bus b, bool muted) override;

        void pause_all () override;
        void resume_all() override;

        void set_position(
            audio::handle handle,
            const float pos[3],
            const float vel[3]) override;

        void set_listener(const interfaces::listener_state &s) override;

    private:
        std::uint32_t next_index_     { 1u };
        std::uint32_t next_generation_{ 1u };
    };
} // namespace cots::audio::backend

#endif //CURSEOFTHESEA_NULL_BACKEND_H
