// Created by Niffoxic (Harsh Dubey)
#include <cots/game_api.h>
#include "game_state.h"
#include "player.h"
#include "audio_test.h"
#include "debug_input.h"

namespace
{
    using game::game_state;

    void on_load(cots::module::memory* mem, const cots::module::services* svc)
    {
        auto* gs = static_cast<game_state*>(mem->permanent);

        gs->services = svc;

        if (!mem->initialized)
        {
            gs->frame_count = 0;
            gs->elapsed     = 0.f;

            game::player_init    (*gs);
            game::audio_test_init(*gs);

            mem->initialized = true;
            svc->log.info("game: first-time init complete");
        }
        else
        {
            svc->log.info("game: hot-reloaded, state preserved");
        }
    }

    void on_unload(cots::module::memory* mem)
    {
        if (auto* gs = static_cast<game_state*>(mem->permanent);
            gs && gs->services)
        {
            gs->services->log.info("game: unloading");
        }
    }

    void update(cots::module::memory* mem, float dt)
    {
        auto* gs = static_cast<game_state*>(mem->permanent);

        gs->frame_count++;
        gs->elapsed += dt;

        game::player_update     (*gs, dt);
        game::audio_test_update (*gs, dt);
        game::debug_input_update(*gs);
    }
}

COTS_GAME_EXPORT void cots_get_game_api(cots::module::api* out)
{
    out->on_load   = &on_load;
    out->on_unload = &on_unload;
    out->update    = &update;
}
