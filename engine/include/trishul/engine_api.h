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
#ifndef CURSEOFTHESEA_ENGINE_API_H
#define CURSEOFTHESEA_ENGINE_API_H

#ifdef BUILDING_ENGINE
    #define FOX_ENGINE_API __declspec(dllexport)
#else
    #define FOX_ENGINE_API __declspec(dllimport)
#endif


#endif //CURSEOFTHESEA_ENGINE_API_H

