// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_ENGINE_SERVICES_H
#define CURSEOFTHESEA_ENGINE_SERVICES_H

#include <cstdint>

namespace cots::module
{
    //~ log
    struct log_services
    {
        void (*info) (const char* msg);
        void (*warn) (const char* msg);
        void (*error)(const char* msg);
    };

    //~ window
    struct window_services
    {
        void (*get_size)          (int* width, int* height);
        bool (*is_focused)        ();
        void (*set_cursor_visible)(bool visible);
        void (*lock_cursor)       ();
        void (*unlock_cursor)     ();
        void (*request_quit)      ();
    };

    //~ input
    enum key_mod : std::uint8_t
    {
        key_mod_none  = 0,
        key_mod_ctrl  = 1 << 0,
        key_mod_shift = 1 << 1,
        key_mod_alt   = 1 << 2,
        key_mod_super = 1 << 3,
    };

    // index into mouse button arrays
    enum mouse_button : std::uint8_t
    {
        mouse_button_left   = 0,
        mouse_button_right  = 1,
        mouse_button_middle = 2,
        mouse_button_x1     = 3,
        mouse_button_x2     = 4,
    };

    struct input_services
    {
        //~ keyboard continuous state
        bool (*key_down)(int vk);
        bool (*key_up)  (int vk);

        //~ keyboard - one frame edges
        bool (*key_pressed) (int vk);
        bool (*key_released)(int vk);

        //~ keyboard - multi-key queries
        bool (*keys_all_down)(const int* vks, int count);
        bool (*keys_any_down)(const int* vks, int count);
        bool (*any_key_pressed)();

        //~ keyboard modifier shortcuts
        bool (*ctrl_down) ();
        bool (*shift_down)();
        bool (*alt_down)  ();
        bool (*super_down)();

        // chord: vk pressed while given mods are held
        bool (*chord)(int vk, std::uint8_t mods, bool strict);

        //~ mouse - buttons
        bool (*mouse_down)    (int button);
        bool (*mouse_pressed) (int button);
        bool (*mouse_released)(int button);

        //~ mouse - position & motion
        void  (*mouse_position) (int* x, int* y);   // client-space pixels
        void  (*mouse_raw_delta)(int* x, int* y);   // raw delta this frame
        float (*mouse_wheel)    ();                 // vertical wheel ticks this frame
    };

    //~ time
    struct time_services
    {
        //~ frame timing
        float (*delta_time)   ();   // seconds since last frame
        float (*delta_time_ms)();   // milliseconds since last frame
        float (*fps)          ();   // current frames per second

        //~ since engine start
        float (*elapsed_time)   ();   // seconds since engine start
        float (*elapsed_time_ms)();   // milliseconds since engine start

        //~ pause control
        void (*pause)    ();
        void (*resume)   ();
        bool (*is_paused)();

        //~ wall-clock
        int (*current_date)(char* out, int out_size);
    };

    //~ audio
    struct audio_handle
    {
        std::uint32_t index;
        std::uint32_t generation;
    };

    enum audio_bus : std::uint8_t
    {
        audio_bus_master = 0,
        audio_bus_sfx    = 1,
        audio_bus_music  = 2,
        audio_bus_ui     = 3,
        audio_bus_voice  = 4,
    };

    struct audio_services
    {
        //~ resource
        bool (*load_sound)  (const char* path, bool positional);
        void (*unload_sound)(const char* path);

        //~ 2D playback
        audio_handle (*play_oneshot)(const char* path, float volume);
        audio_handle (*play_loop)   (const char* path, int bus_index, float volume);

        //~ 3D playback
        audio_handle (*play_3d)     (const char* path, const float pos[3],
                                     float min_distance, float max_distance);
        audio_handle (*play_3d_loop)(const char* path, const float pos[3],
                                     float min_distance, float max_distance, float volume);

        //~ control
        void (*stop)      (audio_handle h, float fade_ms);
        void (*set_volume)(audio_handle h, float volume);
        void (*set_pitch) (audio_handle h, float pitch);
        bool (*is_playing)(audio_handle h);

        //~ 3D
        void (*set_position)(audio_handle h, const float pos[3], const float vel[3]);
        void (*set_listener)(const float pos[3], const float fwd[3], const float up[3]);

        //~ bus & global
        void (*set_bus_volume)(int bus_index, float volume);
        void (*set_bus_muted) (int bus_index, bool muted);
        void (*pause_all) ();
        void (*resume_all)();
    }; // audio services

    //~ render services
    enum mesh_id : std::int32_t
    {
        mesh_id_quad = 0,
        mesh_id_cube = 1,
        mesh_id_ship = 2,
    };

    struct render_services
    {
        void (*set_camera)( const float view[16], const float proj[16],
                            const float pos[3], const float fwd[3],
                            const float up[3]);

        //~ push one instance for this frame cleared each frame
        void (*submit_instance)(int mesh_id, const float world[16], int material_id);
    }; // render services

    //~ editor services
    struct editor_services
    {
        //~ availability
        bool (*enabled)();

        //~ window scoping
        bool (*begin_window)(const char* name);
        void (*end_window)  ();

        //~ widgets
        void (*text)        (const char* msg);
        bool (*button)      (const char* label);
        bool (*checkbox)    (const char* label, bool*  value);
        bool (*slider_float)(const char* label, float* value, float min, float max);
        bool (*slider_int)  (const char* label, int*   value, int   min, int   max);
        void (*separator)   ();

        //~ optional widgets
        bool (*combo)       (const char* label, int* current, const char* const* items, int count);
        bool (*color_edit3) (const char* label, float color[3]);
    }; // editor services

    //~ engine services
    struct engine_services
    {
        void (*get_fps_info)  (std::uint32_t& mt_fps, std::uint32_t& rt_fps);
        void (*set_target_fps)(const std::uint32_t& mt_fps);
    };

    //~ master
    struct services
    {
        engine_services engine;
        log_services    log;
        window_services window;
        input_services  input;
        time_services   time;
        audio_services  audio;
        render_services render;
        editor_services editor;
    };

} // namespace cots::module

#endif //CURSEOFTHESEA_ENGINE_SERVICES_H
