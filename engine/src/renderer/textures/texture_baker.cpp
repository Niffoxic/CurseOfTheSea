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
#include "trishul/renderer/textures/texture_baker.h"
#include "trishul/renderer/utils/image_decode.h"

#include "trishul/utils/logger.h"

#include <chrono>
#include <Windows.h>
#include <wrl/client.h>
#include <DirectXTex.h>

namespace trishul::render::textures
{
    namespace
    {
        //~ picking the bc format for the intent srgb colour goes bc7 normals bc5
        //~ single channel masks bc4 and hdr bc6h
        DXGI_FORMAT bc_format_for(const texture_intent intent) noexcept
        {
            switch (intent)
            {
            case texture_intent::albedo: return DXGI_FORMAT_BC7_UNORM_SRGB;
            case texture_intent::normal: return DXGI_FORMAT_BC5_UNORM;
            case texture_intent::mask:   return DXGI_FORMAT_BC4_UNORM;
            case texture_intent::hdr:    return DXGI_FORMAT_BC6H_UF16;
            }
            return DXGI_FORMAT_BC7_UNORM;
        }

        //~ albedo decodes as srgb everything else stays linear
        DXGI_FORMAT source_format_for(const texture_intent intent) noexcept
        {
            return intent == texture_intent::albedo
                ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
                : DXGI_FORMAT_R8G8B8A8_UNORM;
        }

        //~ compress flags always parallel for albedo we add srgb and take the
        //~ quick bc7 path unless the caller asked for high quality finals
        DirectX::TEX_COMPRESS_FLAGS compress_flags_for(const texture_intent intent,
                                                       const bool high_quality) noexcept
        {
            auto flags = DirectX::TEX_COMPRESS_PARALLEL;
            if (intent == texture_intent::albedo)
            {
                flags = flags | DirectX::TEX_COMPRESS_SRGB;
                if (!high_quality)
                    flags = flags | DirectX::TEX_COMPRESS_BC7_QUICK;
            }
            return flags;
        }
    } //~ anonymous namespace

    bool texture_baker::initialize()
    {
        return true;
    }

    void texture_baker::deinitialize() noexcept
    {
    }

    bool texture_baker::bake(const std::string_view source_path,
                             const texture_intent intent,
                             std::vector<std::uint8_t>& out_dds,
                             const bake_options& opts)
    {
        out_dds.clear();
        using clock = std::chrono::steady_clock;
        const auto t_start = clock::now();

        //~ decoding the source down to rgba8 through stb
        utils::decoded_image src{};
        if (!utils::decode_image_file(source_path, src) || !src.valid())
        {
            LOG_ERROR("[texture-baker] cannot decode '{}'", source_path);
            return false;
        }

        const DXGI_FORMAT src_fmt = source_format_for(intent);
        const DXGI_FORMAT bc_fmt  = bc_format_for(intent);

        //~ wrapping the decoded pixels as a directxtex base image
        DirectX::Image base{};
        base.width      = src.width;
        base.height     = src.height;
        base.format     = src_fmt;
        base.rowPitch   = static_cast<std::size_t>(src.row_pitch());
        base.slicePitch = base.rowPitch * src.height;
        base.pixels     = src.pixels.data();

        //~ generating the mip chain zero levels means all the way down to 1x1
        //~ one level keeps just the base when the editor turns mips off
        LOG_INFO("[texture-baker] generating mips for '{}' {}x{}",
                 source_path, src.width, src.height);
        const auto t_mip_start = clock::now();

        DirectX::ScratchImage mip_chain{};
        if (const HRESULT hr = DirectX::GenerateMipMaps(
                base,
                DirectX::TEX_FILTER_DEFAULT | DirectX::TEX_FILTER_SEPARATE_ALPHA,
                opts.generate_mips ? 0u : 1u,
                mip_chain);
            FAILED(hr))
        {
            LOG_ERROR("[texture-baker] GenerateMipMaps failed for '{}' hr 0x{:08X}",
                      source_path, static_cast<std::uint32_t>(hr));
            return false;
        }
        const auto mip_ms = std::chrono::duration<double, std::milli>(
            clock::now() - t_mip_start).count();
        LOG_INFO("[texture-baker] mips done in {:.1f} ms ({} levels)",
                 mip_ms, mip_chain.GetMetadata().mipLevels);

        //~ compressing to the bc format multithreaded so this can chew a moment
        LOG_INFO("[texture-baker] compressing to {} {} this can take a bit",
                 to_string(intent), opts.high_quality ? "high quality" : "fast");
        const auto t_cmp_start = clock::now();

        DirectX::ScratchImage compressed{};
        if (const HRESULT hr = DirectX::Compress(
                mip_chain.GetImages(),
                mip_chain.GetImageCount(),
                mip_chain.GetMetadata(),
                bc_fmt,
                compress_flags_for(intent, opts.high_quality),
                DirectX::TEX_THRESHOLD_DEFAULT,
                compressed);
            FAILED(hr))
        {
            LOG_ERROR("[texture-baker] Compress failed for '{}' hr 0x{:08X}",
                      source_path, static_cast<std::uint32_t>(hr));
            return false;
        }
        const auto cmp_ms = std::chrono::duration<double, std::milli>(
            clock::now() - t_cmp_start).count();
        LOG_INFO("[texture-baker] compress done in {:.1f} ms", cmp_ms);

        //~ serializing the compressed chain to a dds blob
        DirectX::Blob blob{};
        if (const HRESULT hr = DirectX::SaveToDDSMemory(
                compressed.GetImages(),
                compressed.GetImageCount(),
                compressed.GetMetadata(),
                DirectX::DDS_FLAGS_NONE,
                blob);
            FAILED(hr))
        {
            LOG_ERROR("[texture-baker] SaveToDDSMemory failed for '{}' hr 0x{:08X}",
                      source_path, static_cast<std::uint32_t>(hr));
            return false;
        }

        const auto* bytes = static_cast<const std::uint8_t*>(blob.GetBufferPointer());
        out_dds.assign(bytes, bytes + blob.GetBufferSize());

        const auto total_ms = std::chrono::duration<double, std::milli>(
            clock::now() - t_start).count();

        LOG_INFO("[texture-baker] '{}' baked {}x{} {} mips {} bytes intent {} in {:.1f} ms",
                 source_path, src.width, src.height,
                 compressed.GetMetadata().mipLevels,
                 out_dds.size(), to_string(intent), total_ms);
        return true;
    }
} // namespace trishul::render::textures
