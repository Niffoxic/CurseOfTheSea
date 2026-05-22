// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_SHADER_COMPILER_H
#define CURSEOFTHESEA_SHADER_COMPILER_H

#include <cstdint>
#include <string_view>
#include <vector>
#include <wrl/client.h>
#include <dxcapi.h>

#include "engine/graphics/shaders/vertex_layout.h"

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

    //~ a resource bound by the shader
    struct reflected_binding
    {
        std::string   name           {};
        std::uint32_t bind_point     { 0 };
        std::uint32_t register_space { 0 };
        std::uint32_t bind_count     { 1 };
        std::uint32_t type           { 0 };
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

        //~ compile hlsl to dxil
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
} // namespace cots::graphics::shaders

#endif
