// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_ENGINE_SERVICES_H
#define CURSEOFTHESEA_ENGINE_SERVICES_H
#include <cstdint>

namespace cots::module
{
    struct log_services
    {
        void (*info) (const char* msg);
        void (*warn) (const char* msg);
        void (*error)(const char* msg);
    };

    struct window_services
    {
        void (*get_size)(int* width, int* height);
    };

    struct input_services
    {

    };

    //~ audio services
    struct audio_handle
    {
        std::uint32_t index;
        std::uint32_t generation;
    };

    struct audio_services
    {
        // resource
        bool (*load_sound)  (const char* path, bool positional);
        void (*unload_sound)(const char* path);

        // playback
        audio_handle (*play_oneshot)(const char* path, float volume);
        audio_handle (*play_3d)     (const char* path,
                                     const float pos[3],
                                     float min_distance,
                                     float max_distance);

        void (*stop)      (audio_handle h, float fade_ms);
        void (*set_volume)(audio_handle h, float volume);

        // 3D
        void (*set_position)(audio_handle h, const float pos[3], const float vel[3]);
        void (*set_listener)(const float pos[3], const float fwd[3], const float up[3]);

        // bus + global
        void (*set_bus_volume)(int bus_index, float volume);
        void (*pause_all) ();
        void (*resume_all)();
    };

    struct services
    {
        log_services    log;
        window_services window;
        input_services  input;
        audio_services  audio;
    };
} // namespace cots::module


#endif //CURSEOFTHESEA_ENGINE_SERVICES_H
