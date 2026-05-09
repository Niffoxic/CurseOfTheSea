// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_FMOD_BACKEND_H
#define CURSEOFTHESEA_FMOD_BACKEND_H

#include <array>
#include <unordered_map>
#include <vector>

#include "engine/core/framework/interface/audio_backend.h"

namespace FMOD
{
    class System;
    class Sound;
    class Channel;
    class ChannelGroup;
}

namespace cots::audio::backend
{
    class fmod_backend final: public interface::audio_backend
    {
    public:
         fmod_backend();
        ~fmod_backend() override;

        [[nodiscard]]
        bool initialize  ()          override;
        void deinitialize() noexcept override;

        void begin_update(float dt) override;
        void end_update  ()         override;

        [[nodiscard]]
        bool load_sound  (audio::sound_id id, std::string_view path) override;
        void unload_sound(audio::sound_id id)                        override;

        [[nodiscard]]
        audio::handle play      (audio::sound_id id, const interface::play_params &params)  override;
        void          stop      (audio::handle handle, float fade_ms)                       override;
        void          set_volume(audio::handle handle, float volume)                        override;
        void          set_pitch (audio::handle handle, float pitch)                         override;

        [[nodiscard]]
        bool is_playing(audio::handle handle) const override;

        void set_bus_volume(audio::bus b, float v)    override;
        void set_bus_muted (audio::bus b, bool muted) override;

        void pause_all () override;
        void resume_all() override;
    private:
        struct slot
        {
            FMOD::Channel* channel   { nullptr };
            std::uint32_t  generation{ 0 };
        };

        // resolves a handle to a live channel returns nullptr if stale or finished
        [[nodiscard]] FMOD::Channel* resolve(audio::handle h) const;

        // finds a free slot or recycles a finished one
        [[nodiscard]] std::uint32_t       acquire_slot();
        [[nodiscard]] FMOD::ChannelGroup* group_for   (bus b) const noexcept;

    private:
        static constexpr std::size_t max_voices = 256;

        FMOD::System* system_{ nullptr };

        std::unordered_map<std::uint64_t, FMOD::Sound*> sounds_;
        std::array<slot, max_voices>                    slots_{};

        std::array<FMOD::ChannelGroup*,
                   static_cast<std::size_t>(bus::count)> groups_{};

        std::uint32_t next_generation_{ 1 };
    };
} // namespace cots::audio

#endif //CURSEOFTHESEA_FMOD_BACKEND_H
