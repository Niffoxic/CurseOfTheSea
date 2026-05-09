// Created by Niffoxic (Harsh Dubey)
#include "audio_test.h"
#include "game_state.h"
#include <windows.h>

namespace game
{
    void audio_test_init(game_state& gs)
    {
        const auto& a = gs.services->audio;

        a.load_sound("assets/sfx/test.wav", false);
        a.load_sound("assets/sfx/test.wav", true);

        constexpr float origin[3] { 0.f, 0.f, 0.f };
        gs.audio.ambience = a.play_3d_loop(
            "assets/sfx/test.wav", origin, 2.f, 30.f, 0.6f);
        gs.audio.initialized = true;

        gs.services->log.info("audio_test: playing at origin");
    }

    void audio_test_update(game_state& gs, float dt)
    {
        const auto& in = gs.services->input;
        const auto& a  = gs.services->audio;

        if (in.key_pressed(VK_SPACE))
        {
            a.play_oneshot("assets/sfx/test.wav", 1.0f);
        }

        if (in.key_pressed('E'))
        {
            a.set_pitch(gs.audio.ambience, 1.5f);
        }

        if (in.key_pressed('Q'))
        {
            a.set_pitch(gs.audio.ambience, 1.0f);
        }
    }
}