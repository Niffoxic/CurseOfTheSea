// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/textures/texture_baker.h"
#include "engine/graphics/utils/image_decode.h"

#include <spdlog/spdlog.h>

#include <Windows.h>
#include <wrl/client.h>
#include <DirectXTex.h>

namespace cots::graphics::textures
{
    namespace
    {
        //~ pick the bc format
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

        //~ pick the source format
        DXGI_FORMAT source_format_for(const texture_intent intent) noexcept
        {
            //~ albedo is srgb
            return intent == texture_intent::albedo
                ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
                : DXGI_FORMAT_R8G8B8A8_UNORM;
        }

        //~ compress flags for the intent
        DirectX::TEX_COMPRESS_FLAGS compress_flags_for(const texture_intent intent) noexcept
        {
            //~ srgb aware compress for albedo
            return intent == texture_intent::albedo
                ? DirectX::TEX_COMPRESS_SRGB
                : DirectX::TEX_COMPRESS_DEFAULT;
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
                             std::vector<std::uint8_t>& out_dds)
    {
        out_dds.clear();

        //~ decode the source via stb
        utils::decoded_image src{};
        if (!utils::decode_image_file(source_path, src) || !src.valid())
        {
            spdlog::error("[texture-baker] cannot decode '{}'", source_path);
            return false;
        }

        const DXGI_FORMAT src_fmt = source_format_for(intent);
        const DXGI_FORMAT bc_fmt  = bc_format_for(intent);

        //~ wrap as a directxtex image
        DirectX::Image base{};
        base.width      = src.width;
        base.height     = src.height;
        base.format     = src_fmt;
        base.rowPitch   = static_cast<std::size_t>(src.row_pitch());
        base.slicePitch = base.rowPitch * src.height;
        base.pixels     = src.pixels.data();

        //~ generate the mip chain
        DirectX::ScratchImage mip_chain{};
        if (const HRESULT hr = DirectX::GenerateMipMaps(
                base,
                DirectX::TEX_FILTER_DEFAULT | DirectX::TEX_FILTER_SEPARATE_ALPHA,
                0u, //~ all the way down
                mip_chain);
            FAILED(hr))
        {
            spdlog::error("[texture-baker] GenerateMipMaps failed for '{}' hr 0x{:08X}",
                          source_path, static_cast<std::uint32_t>(hr));
            return false;
        }

        //~ compress to bc
        DirectX::ScratchImage compressed{};
        if (const HRESULT hr = DirectX::Compress(
                mip_chain.GetImages(),
                mip_chain.GetImageCount(),
                mip_chain.GetMetadata(),
                bc_fmt,
                compress_flags_for(intent),
                DirectX::TEX_THRESHOLD_DEFAULT,
                compressed);
            FAILED(hr))
        {
            spdlog::error("[texture-baker] Compress failed for '{}' hr 0x{:08X}",
                          source_path, static_cast<std::uint32_t>(hr));
            return false;
        }

        //~ serialize to a dds blob
        DirectX::Blob blob{};
        if (const HRESULT hr = DirectX::SaveToDDSMemory(
                compressed.GetImages(),
                compressed.GetImageCount(),
                compressed.GetMetadata(),
                DirectX::DDS_FLAGS_NONE,
                blob);
            FAILED(hr))
        {
            spdlog::error("[texture-baker] SaveToDDSMemory failed for '{}' hr 0x{:08X}",
                          source_path, static_cast<std::uint32_t>(hr));
            return false;
        }

        const auto* bytes = static_cast<const std::uint8_t*>(blob.GetBufferPointer());
        out_dds.assign(bytes, bytes + blob.GetBufferSize());

        spdlog::info("[texture-baker] '{}' baked {}x{} {} mips {} bytes intent {}",
                     source_path, src.width, src.height,
                     compressed.GetMetadata().mipLevels,
                     out_dds.size(), to_string(intent));
        return true;
    }
} // namespace cots::graphics::textures
