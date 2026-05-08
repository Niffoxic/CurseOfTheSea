#include <cstdint>
#include <cots/game_api.h>
#include <cstdio>

namespace
{
    struct game_state
    {
        const cots::module::services* services;
        std::uint64_t frame_count;
        float         elapsed;
        float         print_accum;
    };

    void on_load(cots::module::memory* mem, const cots::module::services* svc)
    {
        auto* state = static_cast<game_state*>(mem->permanent);

        state->services = svc;

        if (!mem->initialized)
        {
            state->frame_count = 0;
            state->elapsed     = 0.0f;
            state->print_accum = 0.0f;
            mem->initialized = true;
            svc->log_info("first-time init complete");
        }
        else
        {
            svc->log_info("hot-reloaded: state preserved");
        }
    }

    void on_unload(cots::module::memory* mem)
    {
        auto* state = static_cast<game_state*>(mem->permanent);
        if (state && state->services) state->services->log_info("unloading");
    }

    void update(cots::module::memory* mem, float dt)
    {
        auto* state = static_cast<game_state*>(mem->permanent);

        state->frame_count++;
        state->elapsed     += dt;
        state->print_accum += dt;

        if (state->print_accum >= 1.0f)
        {
            state->print_accum = 0.0f;
            char buf[256];
            std::snprintf(buf, sizeof(buf),
                          "Hi from game.dll!  frame=%llu  elapsed=%.1fs",
                          static_cast<unsigned long long>(state->frame_count),
                          state->elapsed);
            state->services->log_info(buf);
        }
    }
}

COTS_GAME_EXPORT void cots_get_game_api(cots::module::api* out)
{
    out->on_load   = &on_load;
    out->on_unload = &on_unload;
    out->update    = &update;
}
