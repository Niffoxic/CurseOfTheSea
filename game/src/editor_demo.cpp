// Created by Niffoxic (Harsh Dubey)
#include "editor_demo.h"
#include "game_state.h"

namespace game
{
    void editor_demo_update(game_state& gs)
    {
        if (!gs.services) return;
        const auto& ed = gs.services->editor;
        if (!ed.enabled || !ed.enabled()) return;

        if (ed.begin_window("game"))
        {
            ed.text("declared from game dll through the c abi");
            ed.separator();

            //~ live tweak of the player speed
            ed.slider_float("player speed", &gs.player.speed, 0.f, 50.f);

            //~ a button just to prove buttons work
            if (ed.button("reset position"))
            {
                gs.player.pos[0] = 0.f;
                gs.player.pos[1] = 0.f;
                gs.player.pos[2] = 0.f;
            }
        }
        ed.end_window();
    }
} // namespace game
