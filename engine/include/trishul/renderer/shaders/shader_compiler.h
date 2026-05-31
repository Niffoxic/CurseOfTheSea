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
#ifndef CURSEOFTHESEA_SHADER_COMPILER_H
#define CURSEOFTHESEA_SHADER_COMPILER_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <wrl/client.h>
#include <dxcapi.h>

#include "trishul/renderer/shaders/vertex_layout.h"

struct IDxcUtils;
struct IDxcCompiler3;
struct IDxcIncludeHandler;

namespace trishul::render::shaders
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
        std::string_view source_name{ "<memory>" };  //~ shows up in diagnostics
    };

    //~ one resource the shader binds reflected straight out of the bytecode type
    //~ is the raw d3d shader input type enum stuffed into a uint
    struct reflected_binding
    {
        std::string   name           {};
        std::uint32_t bind_point     { 0 };
        std::uint32_t register_space { 0 };
        std::uint32_t bind_count     { 1 };
        std::uint32_t type           { 0 };
    };

    //~ a thin wrapper around dxc turning hlsl into dxil and pulling reflection
    //~ out on the way owns its dxc com objects for the whole lifetime
    class shader_compiler final
    {
    public:
         shader_compiler() = default;
        ~shader_compiler();

        shader_compiler           (const shader_compiler&) = delete;
        shader_compiler& operator=(const shader_compiler&) = delete;

        [[nodiscard]] bool initialize  ();
                      void deinitialize() noexcept;

        //~ compiling hlsl to dxil optionally reflecting the input layout the
        //~ bound resources and any embedded root sig pass null to skip a piece
        [[nodiscard]] bool compile(const shader_compile_desc& desc,
                                   std::vector<std::uint8_t>& out_dxil,
                                   std::vector<vertex_input_element>* out_layout = nullptr,
                                   std::vector<reflected_binding>*    out_bindings = nullptr,
                                   std::vector<std::uint8_t>*         out_embedded_root_sig = nullptr
        ) const;

        [[nodiscard]] static const wchar_t* profile_for(shader_stage stage) noexcept;

    private:
        Microsoft::WRL::ComPtr<IDxcUtils>          utils_;
        Microsoft::WRL::ComPtr<IDxcCompiler3>      compiler_;
        Microsoft::WRL::ComPtr<IDxcIncludeHandler> include_handler_;
    };
} // namespace trishul::render::shaders

#endif //CURSEOFTHESEA_SHADER_COMPILER_H