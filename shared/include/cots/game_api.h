// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_GAME_API_H
#define CURSEOFTHESEA_GAME_API_H

#include <cots/game_memory.h>
#include <cots/engine_services.h>

namespace cots::module
{
    //~ Games gonna be implementing them
    struct api
    {
        // called once after each reload
        void(*on_load)(memory* mem, const services* svc);

        // called before unload: must flush anything important
        void(*on_unload)(memory* mem);

        void(*update)(memory* mem, float dt);
    };
} // namespace cots::module

#define COTS_GAME_EXPORT extern "C" __declspec(dllexport)

// engine looks this up and uses it to populate api struct
COTS_GAME_EXPORT void cots_get_game_api(cots::module::api* api);

#endif //CURSEOFTHESEA_GAME_API_H
