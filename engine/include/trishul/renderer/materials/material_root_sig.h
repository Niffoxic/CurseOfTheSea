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
#ifndef CURSEOFTHESEA_MATERIAL_ROOT_SIG_H
#define CURSEOFTHESEA_MATERIAL_ROOT_SIG_H

#include <wrl/client.h>

struct ID3D12RootSignature;

namespace trishul::render::hardware { class device; }

namespace trishul::render::materials
{
    //~ building the one stable root signature every material pso shares b0 frame
    //~ cb b1 a draw constant b2 material cb t0 instance srv t1 visible srv plus
    //~ the static samplers and the directly indexed heap flag for sm 6 6 see
    //~ root_param in material_layout for the slot numbers
    [[nodiscard]] bool build_material_root_sig(
        hardware::device& device,
        Microsoft::WRL::ComPtr<ID3D12RootSignature>& out_rs);
} // namespace trishul::render::materials

#endif //CURSEOFTHESEA_MATERIAL_ROOT_SIG_H
