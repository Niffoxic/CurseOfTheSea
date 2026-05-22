// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_SHADER_CACHE_H
#define CURSEOFTHESEA_SHADER_CACHE_H

#include <memory>
#include <string_view>

#include "engine/graphics/shaders/shader_compiler.h"
#include "engine/graphics/shaders/shader_storage.h"

namespace cots::graphics::shaders
{
    // view into cached dxil
    // valid for the cache lifetime
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

    class shader_cache final
    {
    public:
        shader_cache() = default;
        ~shader_cache();

        shader_cache           (const shader_cache&) = delete;
        shader_cache& operator=(const shader_cache&) = delete;

        [[nodiscard]]
        bool initialize  (std::unique_ptr<shader_storage> storage);
        void deinitialize() noexcept;   //~ flushes
        void flush       () const;

        //~ reads file at path compiles if missing or stale returns cached dxil view
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
} // namespace cots::graphics::shaders

#endif
