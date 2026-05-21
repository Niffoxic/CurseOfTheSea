// Created by Niffoxic (Harsh Dubey)
#include "player.h"
#include "game_state.h"

namespace game
{
    void player_init(game_state& gs)
    {
        gs.player = {};
        gs.services->log.info("player: initialized");
    }

    void player_update(game_state& gs, float dt)
    {
        const auto& in = gs.services->input;
        auto& p = gs.player;

        const float v = p.speed * dt;
        if (in.key_down('W')) p.pos[2] -= v;
        if (in.key_down('S')) p.pos[2] += v;
        if (in.key_down('A')) p.pos[0] -= v;
        if (in.key_down('D')) p.pos[0] += v;

        // sprint with shift
        if (in.shift_down())
        {
            if (in.key_down('W')) p.pos[2] += v;
            if (in.key_down('S')) p.pos[2] -= v;
            if (in.key_down('A')) p.pos[0] += v;
            if (in.key_down('D')) p.pos[0] -= v;
        }

        const auto& r = gs.services->render;
        if (r.set_camera)
        {
            //~ test
            const float view[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0,
                               -p.pos[0], -p.pos[1], -p.pos[2], 1 };
            const float proj[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
            r.set_camera(view, proj, p.pos, p.forward, p.up);
        }

        // push listener so audio tracks the player
        gs.services->audio.set_listener(p.pos, p.forward, p.up);
    }
} // namespace game
