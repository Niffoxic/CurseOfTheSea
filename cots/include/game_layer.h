//=============================================================================
// Curse of the Sea
//=============================================================================
// Created by  Niffoxic - Harsh Dubey
// Module      WM9M6 Fundamentals of Games Research Development and Management
// Institution University of Warwick
//
// A linear story driven pirate adventure built from scratch in C++23 and
// DirectX 12 for the University of Warwick game project assessment.
//=============================================================================
#ifndef CURSEOFTHESEA_GAME_LAYER_H
#define CURSEOFTHESEA_GAME_LAYER_H

#include <gameplay/layer.h>

#include <trishul/event/window_event.h>

namespace trishul::events { class dispatcher; }

namespace cots
{
    class game_layer final: public gameplay::layer
    {
    public:
         game_layer() = default;
        ~game_layer() override = default;

        void on_attach() override;
        void on_detach() override;
        void on_update(float dt) override;

        void on_window_resized   (const trishul::events::window_resized&);
        void on_window_fullscreen(const trishul::events::window_fullscreen_changed&);
        void on_window_focus     (const trishul::events::window_focus_changed&);
        void on_window_minimized (const trishul::events::window_minimized&);
        void on_window_restored  (const trishul::events::window_restored&);
        void on_window_closed    (const trishul::events::window_closed&);

    private:
        trishul::events::dispatcher* dispatcher_ { nullptr };
    };
} // namespace cots

#endif //CURSEOFTHESEA_GAME_LAYER_H
