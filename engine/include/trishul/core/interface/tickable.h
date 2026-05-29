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
#ifndef CURSEOFTHESEA_TICKABLE_H
#define CURSEOFTHESEA_TICKABLE_H

namespace trishul::interfaces
{
    __interface tickable
    {
        virtual void begin_update(float dt) = 0;
        virtual void end_update  ()         = 0;
    };
} // namespace trishul::interfaces

#endif //CURSEOFTHESEA_TICKABLE_H
