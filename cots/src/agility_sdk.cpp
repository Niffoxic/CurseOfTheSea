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
#include <windows.h>

#ifndef COTS_AGILITY_SDK_VERSION
#  define COTS_AGILITY_SDK_VERSION 616
#endif

extern "C"
{
__declspec(dllexport) extern const UINT     D3D12SDKVersion = COTS_AGILITY_SDK_VERSION;
__declspec(dllexport) extern const char8_t *D3D12SDKPath = u8".\\D3D12\\";
}
