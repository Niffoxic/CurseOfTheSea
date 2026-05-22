// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/shaders/shader_compiler.h"

#include <cots/cots_config.h>
#include <spdlog/spdlog.h>
#include <string>
#include <d3dcompiler.h>

#include "engine/utils/helpers.h"

namespace cots::graphics::shaders
{
    namespace
    {
         DXGI_FORMAT format_from_signature(
             const D3D_REGISTER_COMPONENT_TYPE comp,
             const BYTE mask)
        {
            int n = 0; //~ mask is contiguous from the LSB
            for (int b = 0; b < 4; ++b)
            {
                if (mask & (1u << b)) ++n;
            }

            switch (comp)
            {
            case D3D_REGISTER_COMPONENT_UINT32:
            {
                return  n == 1 ? DXGI_FORMAT_R32_UINT
                        : n == 2 ? DXGI_FORMAT_R32G32_UINT
                        : n == 3 ? DXGI_FORMAT_R32G32B32_UINT
                        : DXGI_FORMAT_R32G32B32A32_UINT;
            }
            case D3D_REGISTER_COMPONENT_SINT32:
            {
                return  n == 1 ? DXGI_FORMAT_R32_SINT
                        : n == 2 ? DXGI_FORMAT_R32G32_SINT
                        : n == 3 ? DXGI_FORMAT_R32G32B32_SINT
                        : DXGI_FORMAT_R32G32B32A32_SINT;
            }
            case D3D_REGISTER_COMPONENT_FLOAT32:
            default:
            {
                return  n == 1 ? DXGI_FORMAT_R32_FLOAT
                        : n == 2 ? DXGI_FORMAT_R32G32_FLOAT
                        : n == 3 ? DXGI_FORMAT_R32G32B32_FLOAT
                        : DXGI_FORMAT_R32G32B32A32_FLOAT;
            }
            }
        }

        bool reflect_input_layout(IDxcUtils* utils, IDxcResult* result,
                                  std::vector<vertex_input_element>& out)
        {
            out.clear();

            Microsoft::WRL::ComPtr<IDxcBlob> refl_blob;
            if (FAILED(result->GetOutput(DXC_OUT_REFLECTION,
                       IID_PPV_ARGS(&refl_blob), nullptr)) || !refl_blob)
            {
                spdlog::error("[shader] no reflection output");
                return false;
            }

            DxcBuffer refl_buf{};
            refl_buf.Ptr      = refl_blob->GetBufferPointer();
            refl_buf.Size     = refl_blob->GetBufferSize();
            refl_buf.Encoding = DXC_CP_ACP;   //~ binary blob

            Microsoft::WRL::ComPtr<ID3D12ShaderReflection> refl;
            if (FAILED(utils->CreateReflection(&refl_buf, IID_PPV_ARGS(&refl))))
            {
                spdlog::error("[shader] CreateReflection failed");
                return false;
            }

            D3D12_SHADER_DESC sd{};
            if (FAILED(refl->GetDesc(&sd)))
            {
                spdlog::error("[shader] reflection GetDesc failed");
                return false;
            }

            out.reserve(sd.InputParameters);
            std::uint32_t slot = 0;
            for (UINT i = 0; i < sd.InputParameters; ++i)
            {
                D3D12_SIGNATURE_PARAMETER_DESC p{};
                if (FAILED(refl->GetInputParameterDesc(i, &p))) continue;

                //~ produced by the IA and
                // only user semantics gets a stream
                if (p.SystemValueType != D3D_NAME_UNDEFINED) continue;

                vertex_input_element el{};
                el.semantic_name  = p.SemanticName ? p.SemanticName : "";
                el.semantic_index = p.SemanticIndex;
                el.format         = static_cast<std::uint32_t>(
                                        format_from_signature(p.ComponentType, p.Mask));
                el.input_slot     = slot++;
                out.push_back(std::move(el));
            }
            return true;
        }
    } // anonymouse namespace

    shader_compiler::~shader_compiler() { deinitialize(); }

    bool shader_compiler::initialize()
    {
        if (FAILED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils_))))
        {
            spdlog::error("[shader] DxcCreateInstance(Utils) failed");
            return false;
        }
        if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler_))))
        {
            spdlog::error("[shader] DxcCreateInstance(Compiler) failed");
            return false;
        }
        if (FAILED(utils_->CreateDefaultIncludeHandler(&include_handler_)))
        {
            spdlog::error("[shader] CreateDefaultIncludeHandler failed");
            return false;
        }
        spdlog::info("[shader] DXC compiler ready");
        return true;
    }

    void shader_compiler::deinitialize() noexcept
    {
        include_handler_.Reset();
        compiler_       .Reset();
        utils_          .Reset();
    }

    const wchar_t* shader_compiler::profile_for(const shader_stage stage) noexcept
    {
        switch (stage)
        {
            case shader_stage::vertex:        return L"vs_6_6";
            case shader_stage::pixel:         return L"ps_6_6";
            case shader_stage::compute:       return L"cs_6_6";
            case shader_stage::geometry:      return L"gs_6_6";
            case shader_stage::hull:          return L"hs_6_6";
            case shader_stage::domain:        return L"ds_6_6";
            case shader_stage::amplification: return L"as_6_6";
            case shader_stage::mesh:          return L"ms_6_6";
            default:                          return L"vs_6_6";
        }
    }

    bool shader_compiler::compile(
        const shader_compile_desc &desc,
        std::vector<std::uint8_t> &out_dxil,
        std::vector<vertex_input_element> *out_layout) const
    {
        if (!compiler_) return false;

        const std::wstring entry   = helpers::to_wide(desc.entry_point);
        const std::wstring profile = profile_for(desc.stage);
        const std::wstring name    =  helpers::to_wide(desc.source_name);

        std::vector<LPCWSTR> args;
        args.push_back(name.c_str());          //~ shows up in diagnostics
        args.push_back(L"-E"); args.push_back(entry.c_str());
        args.push_back(L"-T"); args.push_back(profile.c_str());
        args.push_back(L"-HV"); args.push_back(L"2021");   //~ HLSL 2021
#if COTS_DEBUG
        args.push_back(L"-Zi");
        args.push_back(L"-Od");
        args.push_back(DXC_ARG_DEBUG_NAME_FOR_SOURCE);
        args.push_back(L"-Qembed_debug");
#else
        args.push_back(L"-O3");
#endif

        DxcBuffer src{};
        src.Ptr      = desc.source.data();
        src.Size     = desc.source.size();
        src.Encoding = DXC_CP_UTF8;

        Microsoft::WRL::ComPtr<IDxcResult> result;
        if (FAILED(compiler_->Compile(&src, args.data(),
                                      static_cast<UINT32>(args.size()),
                                      include_handler_.Get(),
                                      IID_PPV_ARGS(&result))))
        {
            spdlog::error("[shader] Compile() call failed for {}", desc.source_name);
            return false;
        }

        Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors;
        result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
        if (errors && errors->GetStringLength() > 0)
        {
            spdlog::error("[shader] {}:\n{}", desc.source_name, errors->GetStringPointer());
        }

        HRESULT status = E_FAIL;
        result->GetStatus(&status);
        if (FAILED(status)) return false;

        Microsoft::WRL::ComPtr<IDxcBlob> object;
        if (FAILED(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&object), nullptr)) || !object)
        {
            spdlog::error("[shader] no object output for {}", desc.source_name);
            return false;
        }

        const auto* bytes = static_cast<const std::uint8_t*>(object->GetBufferPointer());
        out_dxil.assign(bytes, bytes + object->GetBufferSize());

        if (out_layout)   //~ reflect the SoA input layout
        {
            if (!reflect_input_layout(utils_.Get(), result.Get(), *out_layout))
            {
                spdlog::warn("[shader] input-layout reflection failed for {}",
                desc.source_name);
            }
        }
        return true;
    }
} // namespace cots::graphics::shaders
