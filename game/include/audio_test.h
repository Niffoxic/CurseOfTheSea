// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_AUDIO_TEST_H
#define CURSEOFTHESEA_AUDIO_TEST_H

namespace game
{
    struct game_state;

    void audio_test_init  (game_state& gs);
    void audio_test_update(game_state& gs, float dt);
}

#endif //CURSEOFTHESEA_AUDIO_TEST_H
