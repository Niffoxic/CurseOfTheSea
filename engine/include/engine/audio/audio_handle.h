// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_AUDIO_HANDLE_H
#define CURSEOFTHESEA_AUDIO_HANDLE_H

#include <cstdint>

namespace cots::audio
{
    struct handle
    {
        std::uint32_t index     { 0u };
        std::uint32_t generation{ 0u };

        [[nodiscard]] bool valid() const noexcept
        {
            return generation != 0u;
        }

        [[nodiscard]] static handle invalid() noexcept
        {
            return handle{ 0u, 0u };
        }

        bool operator==(const handle& other) const noexcept
        {
            return index == other.index && generation == other.generation;
        }
    };
} // namespace cots::audio

#endif //CURSEOFTHESEA_AUDIO_HANDLE_H
