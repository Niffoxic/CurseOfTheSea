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

namespace cots
{
    class game_layer final: public gameplay::layer
    {
    public:
        game_layer();

        void on_attach() override;
        void on_detach() override;
        void on_update(float dt) override;
    };
} // namespace cots

#endif //CURSEOFTHESEA_GAME_LAYER_H
