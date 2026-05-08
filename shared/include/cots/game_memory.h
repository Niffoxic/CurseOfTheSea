// Created by Niffoxic (Harsh Dubey)
// Shared between the engine and the game
#ifndef CURSEOFTHESEA_GAME_MEMORY_H
#define CURSEOFTHESEA_GAME_MEMORY_H

#include <cstddef>

namespace cots::module
{
    struct memory
    {
        void* permanent; // persistent game state
        void* transient; // per-frame stuff
        std::size_t permanent_size;
        std::size_t transient_size;
        bool initialized;
    };
} // namespace cots::module

#endif //CURSEOFTHESEA_GAME_MEMORY_H
