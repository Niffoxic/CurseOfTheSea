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
#include <gameplay/application.h>
#include "game_layer.h"

#include <memory>
#include <windows.h>

int WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    gameplay::application app{};
    app.push_layer(std::make_unique<cots::game_layer>());
    return app.run();
}
