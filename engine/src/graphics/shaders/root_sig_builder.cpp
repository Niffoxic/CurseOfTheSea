// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/shaders/root_sig_builder.h"

#include <d3d12.h>
#include <spdlog/spdlog.h>
#include <wrl/client.h>

#include <algorithm>
#include <unordered_map>

namespace cots::graphics::shaders
{
    namespace
    {
        //~ key for a unique cbv binding
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

        //~ standard static sampler set
        void fill_static_samplers(D3D12_STATIC_SAMPLER_DESC out[6]) noexcept
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
        }
    } //~ anonymous namespace

    bool build_program_root_sig(std::span<const shader_bytecode> stages,
                                std::vector<std::uint8_t>& out_blob)
    {
        out_blob.clear();

        //~ shader override wins
        for (const auto& s : stages)
        {
            if (s.embedded_root_sig && !s.embedded_root_sig->empty())
            {
                out_blob = *s.embedded_root_sig;
                spdlog::info("[root-sig] using embedded blob {} bytes", out_blob.size());
                return true;
            }
        }

        //~ union of cbv bindings across stages
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

        //~ stable order so the serialized blob is deterministic
        std::vector<cbv_key> ordered;
        ordered.reserve(cbvs.size());
        for (const auto& [k, _] : cbvs) ordered.push_back(k);
        std::sort(ordered.begin(), ordered.end(), [](const cbv_key& a, const cbv_key& b)
        {
            if (a.space != b.space) return a.space < b.space;
            return a.bind_point < b.bind_point;
        });

        //~ root parameters one cbv each
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

        //~ static samplers
        D3D12_STATIC_SAMPLER_DESC samplers[6]{};
        fill_static_samplers(samplers);

        //~ root flags input layout and bindless heap
        const D3D12_ROOT_SIGNATURE_FLAGS flags =
              D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
            | D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC vdesc{};
        vdesc.Version                       = D3D_ROOT_SIGNATURE_VERSION_1_1;
        vdesc.Desc_1_1.NumParameters        = static_cast<UINT>(params.size());
        vdesc.Desc_1_1.pParameters          = params.empty() ? nullptr : params.data();
        vdesc.Desc_1_1.NumStaticSamplers    = 6u;
        vdesc.Desc_1_1.pStaticSamplers      = samplers;
        vdesc.Desc_1_1.Flags                = flags;

        Microsoft::WRL::ComPtr<ID3DBlob> blob;
        Microsoft::WRL::ComPtr<ID3DBlob> err;
        if (FAILED(D3D12SerializeVersionedRootSignature(&vdesc, &blob, &err)))
        {
            if (err)
            {
                spdlog::error("[root-sig] serialize failed {}",
                              static_cast<const char*>(err->GetBufferPointer()));
            }
            else
            {
                spdlog::error("[root-sig] serialize failed");
            }
            return false;
        }

        const auto* bytes = static_cast<const std::uint8_t*>(blob->GetBufferPointer());
        out_blob.assign(bytes, bytes + blob->GetBufferSize());

        spdlog::info("[root-sig] built {} cbv params six static samplers {} bytes",
                     params.size(), out_blob.size());
        return true;
    }
} // namespace cots::graphics::shaders
