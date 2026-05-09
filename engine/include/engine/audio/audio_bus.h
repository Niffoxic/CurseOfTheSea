// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_AUDIO_BUS_H
#define CURSEOFTHESEA_AUDIO_BUS_H

#include <cstdint>

namespace cots::audio
{
    enum class bus : std::uint8_t
    {
        master = 0,
        sfx,
        music,
        ui,
        voice,
        count,
    };
} // namespace cots::audio

#endif //CURSEOFTHESEA_AUDIO_BUS_H
