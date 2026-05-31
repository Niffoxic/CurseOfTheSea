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
#include "trishul/renderer/shaders/root_sig_builder.h"

#include <d3d12.h>
#include <wrl/client.h>

#include <algorithm>
#include <unordered_map>

#include "trishul/utils/logger.h"

namespace trishul::render::shaders
{
    namespace
    {
        //~ keying a cbv by its register and space so duplicates across stages
        //~ collapse to one
        struct cbv_key
        {
            std::uint32_t bind_point;
            std::uint32_t space;

            bool operator==(const cbv_key& o) const noexcept
            {
                return bind_point == o.bind_point && space == o.space;
            }
        };

        struct cbv_key_hash
        {
            std::size_t operator()(const cbv_key& k) const noexcept
            {
                return (static_cast<std::size_t>(k.bind_point) << 16) ^ k.space;
            }
        };

        //~ the engine static sampler set six everyday ones plus a shadow compare
        //~ sampler at s6 the compare one turns on hardware pcf so the same shader
        //~ patterns work everywhere sky tonemap volumetrics all pick it up free
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

            //~ the compare sampler hardware pcf the white border reads receivers
            //~ past the cascade as lit border clamps stray taps to the edge
            D3D12_STATIC_SAMPLER_DESC& cmp = out[6];
            cmp.Filter           = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
            cmp.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            cmp.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            cmp.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            cmp.MipLODBias       = 0.0f;
            cmp.MaxAnisotropy    = 0u;
            cmp.ComparisonFunc   = D3D12_COMPARISON_FUNC_LESS_EQUAL;
            cmp.BorderColor      = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
            cmp.MinLOD           = 0.0f;
            cmp.MaxLOD           = D3D12_FLOAT32_MAX;
            cmp.ShaderRegister   = 6u;
            cmp.RegisterSpace    = 0;
            cmp.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        }
    } //~ anonymous namespace

    bool build_program_root_sig(std::span<const shader_bytecode> stages,
                                std::vector<std::uint8_t>& out_blob)
    {
        out_blob.clear();

        //~ a shaders own embedded root sig always wins grab it and bail
        for (const auto& s : stages)
        {
            if (s.embedded_root_sig && !s.embedded_root_sig->empty())
            {
                out_blob = *s.embedded_root_sig;
                LOG_INFO("[root-sig] using embedded blob {} bytes", out_blob.size());
                return true;
            }
        }

        //~ otherwise gathering the union of cbv bindings across every stage
        constexpr std::uint32_t k_sit_cbuffer = 0;

        std::unordered_map<cbv_key, std::uint32_t, cbv_key_hash> cbvs;
        for (const auto& s : stages)
        {
            if (!s.bindings) continue;
            for (const auto& b : *s.bindings)
            {
                if (b.type != k_sit_cbuffer) continue;
                cbvs.emplace(cbv_key{ b.bind_point, b.register_space }, 0u);
            }
        }

        //~ sorting them so the serialized blob comes out identical every run
        std::vector<cbv_key> ordered;
        ordered.reserve(cbvs.size());
        for (const auto& [k, _] : cbvs) ordered.push_back(k);
        std::sort(ordered.begin(), ordered.end(), [](const cbv_key& a, const cbv_key& b)
        {
            if (a.space != b.space) return a.space < b.space;
            return a.bind_point < b.bind_point;
        });

        //~ one root cbv parameter per binding
        std::vector<D3D12_ROOT_PARAMETER1> params;
        params.reserve(ordered.size());
        for (const auto& k : ordered)
        {
            D3D12_ROOT_PARAMETER1 p{};
            p.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
            p.Descriptor.ShaderRegister = k.bind_point;
            p.Descriptor.RegisterSpace  = k.space;
            p.Descriptor.Flags          = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
            p.ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;
            params.push_back(p);
        }

        //~ the static samplers
        D3D12_STATIC_SAMPLER_DESC samplers[k_sampler_count]{};
        fill_static_samplers(samplers);

        //~ flags letting the IA feed the input layout and the shaders index the
        //~ bindless heap directly
        const D3D12_ROOT_SIGNATURE_FLAGS flags =
              D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
            | D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC vdesc{};
        vdesc.Version                       = D3D_ROOT_SIGNATURE_VERSION_1_1;
        vdesc.Desc_1_1.NumParameters        = static_cast<UINT>(params.size());
        vdesc.Desc_1_1.pParameters          = params.empty() ? nullptr : params.data();
        vdesc.Desc_1_1.NumStaticSamplers    = k_sampler_count;
        vdesc.Desc_1_1.pStaticSamplers      = samplers;
        vdesc.Desc_1_1.Flags                = flags;

        Microsoft::WRL::ComPtr<ID3DBlob> blob;
        Microsoft::WRL::ComPtr<ID3DBlob> err;
        if (FAILED(D3D12SerializeVersionedRootSignature(&vdesc, &blob, &err)))
        {
            if (err)
            {
                LOG_ERROR("[root-sig] serialize failed {}",
                          static_cast<const char*>(err->GetBufferPointer()));
            }
            else
            {
                LOG_ERROR("[root-sig] serialize failed");
            }
            return false;
        }

        const auto* bytes = static_cast<const std::uint8_t*>(blob->GetBufferPointer());
        out_blob.assign(bytes, bytes + blob->GetBufferSize());

        LOG_INFO("[root-sig] built {} cbv params {} static samplers {} bytes",
                 params.size(), k_sampler_count, out_blob.size());
        return true;
    }
} // namespace trishul::render::shaders