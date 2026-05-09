// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_PLAYER_H
#define CURSEOFTHESEA_PLAYER_H

namespace game
{
    struct game_state;

    void player_init  (game_state& gs);
    void player_update(game_state& gs, float dt);
}

#endif //CURSEOFTHESEA_PLAYER_H
