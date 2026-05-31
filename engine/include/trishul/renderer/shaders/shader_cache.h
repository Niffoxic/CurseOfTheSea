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
#ifndef CURSEOFTHESEA_SHADER_CACHE_H
#define CURSEOFTHESEA_SHADER_CACHE_H

#include <memory>
#include <string_view>

#include "trishul/renderer/shaders/shader_compiler.h"
#include "trishul/renderer/shaders/shader_storage.h"

namespace trishul::render::shaders
{
    //~ a borrowed view into cached dxil plus its reflection only good while the
    //~ cache keeps that entry alive so do not hang onto it across a clear or a
    //~ recompile
    struct shader_bytecode
    {
        const std::uint8_t* data { nullptr };
        std::size_t         size { 0 };

        const std::vector<vertex_input_element>* input_layout       { nullptr };
        const std::vector<reflected_binding>*    bindings           { nullptr };
        const std::vector<std::uint8_t>*         embedded_root_sig  { nullptr };

        [[nodiscard]] bool valid() const noexcept
        {
            return data && size;
        }
    };

    //~ the front door for shaders handing back cached dxil or compiling on a
    //~ miss owns the compiler and a swappable storage strategy
    class shader_cache final
    {
    public:
        shader_cache() = default;
        ~shader_cache();

        shader_cache           (const shader_cache&) = delete;
        shader_cache& operator=(const shader_cache&) = delete;

        [[nodiscard]]
        bool initialize  (std::unique_ptr<shader_storage> storage);
        void deinitialize() noexcept;   //~ flushing on the way out
        void flush       () const;

        //~ reading the file at path compiling on a miss or a stale hit then
        //~ handing back a view into the cached dxil
        [[nodiscard]] shader_bytecode get_or_compile(
            std::string_view path,
            std::string_view entry,
            shader_stage     stage
        );

        void clear();
        bool recompile(std::uint64_t key);

    private:
        shader_compiler                  compiler_;
        std::unique_ptr<shader_storage>  storage_;
        cache_map                        entries_;
    };
} // namespace trishul::render::shaders

#endif //CURSEOFTHESEA_SHADER_CACHE_H