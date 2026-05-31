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
#include "trishul/renderer/materials/material_root_sig.h"

#include "trishul/renderer/hardware/device.h"
#include "trishul/utils/logger.h"

#include <d3d12.h>

namespace trishul::render::materials
{
    namespace
    {
        //~ the static sampler set mirrors the shader root_sig_builder so the
        //~ same shader patterns work under either root sig six everyday samplers
        //~ at s0..s5 plus a shadow compare sampler at s6 the compare one turns
        //~ on hardware pcf the white border reads out of range shadow taps as
        //~ lit so casters never wrap around the cascade edge
        constexpr std::uint32_t k_sampler_count = 7u;
        void fill_static_samplers(D3D12_STATIC_SAMPLER_DESC out[k_sampler_count]) noexcept
        {
            auto make = [](const std::uint32_t reg,
                           const D3D12_FILTER filter,
                           const D3D12_TEXTURE_ADDRESS_MODE addr,
                           const UINT max_aniso = 1u)
            {
                D3D12_STATIC_SAMPLER_DESC s{};
                s.Filter           = filter;
                s.AddressU         = addr;
                s.AddressV         = addr;
                s.AddressW         = addr;
                s.MipLODBias       = 0.0f;
                s.MaxAnisotropy    = max_aniso;
                s.ComparisonFunc   = D3D12_COMPARISON_FUNC_NEVER;
                s.BorderColor      = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
                s.MinLOD           = 0.0f;
                s.MaxLOD           = D3D12_FLOAT32_MAX;
                s.ShaderRegister   = reg;
                s.RegisterSpace    = 0;
                s.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
                return s;
            };

            out[0] = make(0u, D3D12_FILTER_MIN_MAG_MIP_POINT,  D3D12_TEXTURE_ADDRESS_MODE_WRAP);
            out[1] = make(1u, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
            out[2] = make(2u, D3D12_FILTER_ANISOTROPIC,        D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8u);
            out[3] = make(3u, D3D12_FILTER_MIN_MAG_MIP_POINT,  D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
            out[4] = make(4u, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
            out[5] = make(5u, D3D12_FILTER_ANISOTROPIC,        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8u);

            //~ the shadow compare sampler linear filter gives hardware 2x2 pcf on
            //~ SampleCmpLevelZero border addressing with a white border reads out
            //~ of range receivers as lit
            D3D12_STATIC_SAMPLER_DESC& cmp = out[6];
            cmp = {};
            cmp.Filter           = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
            cmp.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            cmp.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            cmp.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            cmp.MipLODBias       = 0.0f;
            cmp.MaxAnisotropy    = 1u;
            cmp.ComparisonFunc   = D3D12_COMPARISON_FUNC_LESS_EQUAL;
            cmp.BorderColor      = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
            cmp.MinLOD           = 0.0f;
            cmp.MaxLOD           = D3D12_FLOAT32_MAX;
            cmp.ShaderRegister   = 6u;
            cmp.RegisterSpace    = 0;
            cmp.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        }
    } //~ anonymous namespace

    bool build_material_root_sig(hardware::device& device,
                                 Microsoft::WRL::ComPtr<ID3D12RootSignature>& out_rs)
    {
        auto* d3d = device.d3d12_device();
        if (!d3d)
        {
            LOG_ERROR("[material-rs] no device");
            return false;
        }

        //~ five params b0 frame cb shared b1 one uint visible_base vs only b2
        //~ material cb shared t0 instance srv vs only t1 visible srv vs only the
        //~ ExecuteIndirect command signature sets b1 per draw and the cpu sets
        //~ it via SetGraphicsRoot32BitConstants before each bucket both land on
        //~ the same b1 slot
        D3D12_ROOT_PARAMETER1 params[5] {};

        //~ b0 FrameCB shared across stages
        params[0].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
        params[0].Descriptor.ShaderRegister = 0u;
        params[0].Descriptor.RegisterSpace  = 0u;
        params[0].Descriptor.Flags          = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
        params[0].ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;

        //~ b1 one uint root constant the visible_base offset
        params[1].ParameterType                = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        params[1].Constants.ShaderRegister     = 1u;
        params[1].Constants.RegisterSpace      = 0u;
        params[1].Constants.Num32BitValues     = 1u;
        params[1].ShaderVisibility             = D3D12_SHADER_VISIBILITY_VERTEX;

        //~ b2 MaterialCB
        params[2].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
        params[2].Descriptor.ShaderRegister = 2u;
        params[2].Descriptor.RegisterSpace  = 0u;
        params[2].Descriptor.Flags          = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
        params[2].ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;

        //~ t0 instance structured buffer root srv vs only
        params[3].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_SRV;
        params[3].Descriptor.ShaderRegister = 0u;
        params[3].Descriptor.RegisterSpace  = 0u;
        params[3].Descriptor.Flags          = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
        params[3].ShaderVisibility          = D3D12_SHADER_VISIBILITY_VERTEX;

        //~ t1 visible index buffer root srv vs only
        params[4].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_SRV;
        params[4].Descriptor.ShaderRegister = 1u;
        params[4].Descriptor.RegisterSpace  = 0u;
        params[4].Descriptor.Flags          = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
        params[4].ShaderVisibility          = D3D12_SHADER_VISIBILITY_VERTEX;

        D3D12_STATIC_SAMPLER_DESC samplers[k_sampler_count] {};
        fill_static_samplers(samplers);

        const D3D12_ROOT_SIGNATURE_FLAGS flags =
              D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
            | D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC vdesc{};
        vdesc.Version                       = D3D_ROOT_SIGNATURE_VERSION_1_1;
        vdesc.Desc_1_1.NumParameters        = 5u;
        vdesc.Desc_1_1.pParameters          = params;
        vdesc.Desc_1_1.NumStaticSamplers    = k_sampler_count;
        vdesc.Desc_1_1.pStaticSamplers      = samplers;
        vdesc.Desc_1_1.Flags                = flags;

        Microsoft::WRL::ComPtr<ID3DBlob> blob;
        Microsoft::WRL::ComPtr<ID3DBlob> err;
        if (FAILED(D3D12SerializeVersionedRootSignature(&vdesc, &blob, &err)))
        {
            if (err)
            {
                LOG_ERROR("[material-rs] serialize failed {}",
                          static_cast<const char*>(err->GetBufferPointer()));
            }
            else
            {
                LOG_ERROR("[material-rs] serialize failed");
            }
            return false;
        }

        out_rs.Reset();
        if (FAILED(d3d->CreateRootSignature(0,
                                            blob->GetBufferPointer(),
                                            blob->GetBufferSize(),
                                            IID_PPV_ARGS(&out_rs))))
        {
            LOG_ERROR("[material-rs] create root sig failed");
            return false;
        }
        (void)out_rs->SetName(L"material root sig");

        LOG_INFO("[material-rs] built 5 params {} samplers bindless flag {} bytes",
                 k_sampler_count, blob->GetBufferSize());
        return true;
    }
} // namespace trishul::render::materials
