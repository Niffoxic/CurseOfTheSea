// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_SOUND_ID_H
#define CURSEOFTHESEA_SOUND_ID_H

#include <cstdint>
#include <string_view>

namespace cots::audio
{
    struct sound_id
    {
        std::uint64_t value{ 0u };

        bool operator==(const sound_id& other) const noexcept
        {
            return value == other.value;
        }
    };

    consteval sound_id make_sound_id(const std::string_view name) noexcept
    {
        std::uint64_t h = 0xcbf29ce484222325ull;
        for (const char c: name)
        {
            h ^= static_cast<std::uint64_t>(static_cast<unsigned char>(c));
            h *= 0x100000001b3ull;
        }

        return sound_id{ h };
    }

    [[nodiscard]] inline sound_id hash_sound_id(std::string_view name) noexcept
    {
        std::uint64_t h = 0xcbf29ce484222325ull;
        for (const char c : name)
        {
            h ^= static_cast<std::uint64_t>(static_cast<unsigned char>(c));
            h *= 0x100000001b3ull;
        }
        return sound_id{ h };
    }
} // namespace cots::audio

namespace std
{
    template<> struct hash<cots::audio::sound_id>
    {
        std::size_t operator()(const cots::audio::sound_id& id) const noexcept
        {
            return id.value;
        }
    };
} // namespace std

#endif //CURSEOFTHESEA_SOUND_ID_H
