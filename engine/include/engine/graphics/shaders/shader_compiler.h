// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_SHADER_COMPILER_H
#define CURSEOFTHESEA_SHADER_COMPILER_H

#include <cstdint>
#include <string_view>
#include <vector>
#include <wrl/client.h>
#include <dxcapi.h>

struct IDxcUtils;
struct IDxcCompiler3;
struct IDxcIncludeHandler;

namespace cots::graphics::shaders
{
    enum class shader_stage : std::uint8_t
    {
        vertex,
        pixel,
        compute,
        geometry,
        hull,
        domain,
        amplification,
        mesh
    };

    struct shader_compile_desc
    {
        std::string_view source;
        std::string_view entry_point;
        shader_stage     stage      { shader_stage::vertex };
        std::string_view source_name{ "<memory>" };  //~ for diagnostics
    };

    class shader_compiler final
    {
    public:
         shader_compiler() = default;
        ~shader_compiler();

        shader_compiler           (const shader_compiler&) = delete;
        shader_compiler& operator=(const shader_compiler&) = delete;

        [[nodiscard]] bool initialize  ();
                      void deinitialize() noexcept;

        //~ compiles hlsl to dxil
        [[nodiscard]] bool compile(const shader_compile_desc& desc,
                                   std::vector<std::uint8_t>& out_dxil) const;

        [[nodiscard]] static const wchar_t* profile_for(shader_stage stage) noexcept;

    private:
        Microsoft::WRL::ComPtr<IDxcUtils>          utils_;
        Microsoft::WRL::ComPtr<IDxcCompiler3>      compiler_;
        Microsoft::WRL::ComPtr<IDxcIncludeHandler> include_handler_;
    };
} // namespace cots::graphics::shaders

#endif
