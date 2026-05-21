// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/shaders/shader_compiler.h"

#include <cots/cots_config.h>
#include <spdlog/spdlog.h>
#include <dxcapi.h>
#include <string>

#include "engine/utils/helpers.h"

namespace cots::graphics::shaders
{
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

    bool shader_compiler::compile(const shader_compile_desc& desc,
                                  std::vector<std::uint8_t>& out_dxil) const
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
        return true;
    }
} // namespace cots::graphics::shaders
