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
#ifndef CURSEOFTHESEA_ISUBSYSTEMS_H
#define CURSEOFTHESEA_ISUBSYSTEMS_H
#include <string>

namespace trishul::interfaces
{
    __interface subsystems
    {
        //~ lifecycle
        [[nodiscard]]
        virtual bool initialize  ();
        virtual void deinitialize() noexcept;

        [[nodiscard]]
        std::string_view name() const noexcept;
    };
} // namespace trishul::interfaces

#endif //CURSEOFTHESEA_ISUBSYSTEMS_H
