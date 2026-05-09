// Created by Niffoxic (Harsh Dubey)
#include "debug_input.h"
#include "game_state.h"
#include <windows.h>

namespace game
{
    void debug_input_update(game_state& gs)
    {
        const auto& in  = gs.services->input;
        const auto& aud = gs.services->audio;
        const auto& win = gs.services->window;
        const auto& log = gs.services->log;

        // ESC quits
        if (in.key_pressed(VK_ESCAPE))
        {
            log.info("debug: quit requested");
            win.request_quit();
        }

        if (in.key_pressed('M'))
        {
            static bool muted = false;
            muted = !muted;
            aud.set_bus_muted(cots::module::audio_bus_master, muted);
            log.info(muted ? "debug: muted" : "debug: unmuted");
        }

        if (in.chord('P', cots::module::key_mod_ctrl, true))
        {
            aud.pause_all();
            log.info("debug: audio paused");
        }
        if (in.chord('P', cots::module::key_mod_ctrl |
                          cots::module::key_mod_shift, true))
        {
            aud.resume_all();
            log.info("debug: audio resumed");
        }
    }
} // namespace game
