// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_GAME_STATE_H
#define CURSEOFTHESEA_GAME_STATE_H

#include <cstdint>
#include <cots/engine_services.h>

namespace game
{
    struct player_state
    {
        float pos[3]    { 0.f, 0.f,  0.f };
        float forward[3]{ 0.f, 0.f, -1.f };
        float up[3]     { 0.f, 1.f,  0.f };
        float speed     { 5.f };
    };

    struct audio_test_state
    {
        cots::module::audio_handle ambience{};
        bool initialized{ false };
    };

    struct game_state
    {
        const cots::module::services* services{ nullptr };

        std::uint64_t frame_count{ 0 };
        float         elapsed    { 0.f };

        player_state     player;
        audio_test_state audio;
    };
}

#endif //CURSEOFTHESEA_GAME_STATE_H
