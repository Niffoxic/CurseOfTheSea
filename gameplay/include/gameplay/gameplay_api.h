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
#ifndef CURSEOFTHESEA_GAMEPLAY_API_H
#define CURSEOFTHESEA_GAMEPLAY_API_H

#ifndef BUILDING_GAMEPLAY
    #define GP_API __declspec(dllexport)
#else
    #define GP_API __declspec(dllimport)
#endif

#endif //CURSEOFTHESEA_GAMEPLAY_API_H
