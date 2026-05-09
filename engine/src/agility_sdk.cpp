// Created by Niffoxic (Harsh Dubey)
#include <cots/cots_config.h>
#include <windows.h>

#ifndef COTS_AGILITY_SDK_VERSION
#  define COTS_AGILITY_SDK_VERSION 616
#endif

extern "C"
{
__declspec(dllexport) extern const UINT     D3D12SDKVersion = COTS_AGILITY_SDK_VERSION;
__declspec(dllexport) extern const char8_t *D3D12SDKPath = u8".\\D3D12\\";
}
