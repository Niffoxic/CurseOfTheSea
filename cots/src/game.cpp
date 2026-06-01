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
#include "resource.h"

#include <trishul/engine.h>

#include <memory>
#include <windows.h>

int WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    trishul::engine_create_info info{};
    info.window_title     = L"Curse of the Sea";
    info.window_width     = 1280;
    info.window_height    = 720;
    info.icon_resource_id = IDI_APP_ICON;
    info.icon_path        = L"deco/app.ico";
    info.target_fps       = 360; //~ target main thread tho I dont think more than 60 is needed xD but for flexing 360

    gameplay::application app{ info };
    app.push_layer(std::make_unique<trishul::game_layer>());
    return app.run();
}
