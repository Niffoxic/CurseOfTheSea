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
#include "trishul/renderer/shaders/storage/binary_storage.h"

#include <filesystem>
#include <fstream>

#include "trishul/utils/logger.h"

namespace trishul::render::shaders
{
    namespace
    {
        constexpr std::uint32_t k_magic   = 0x31435343;
        constexpr std::uint32_t k_version = 4;

        //~ sanity caps a corrupt or truncated file can hand us a wild length and
        //~ since we run no exceptions a blind resize on garbage would just kill
        //~ the process so we refuse anything past a sane ceiling and bail clean
        constexpr std::uint32_t k_max_bytes    = 64u * 1024u * 1024u; //~ any one blob 64 MB
        constexpr std::uint32_t k_max_string   = 1u << 16;            //~ any name or id 64 KB
        constexpr std::uint32_t k_max_elements = 1u << 16;            //~ layout or binding count

        template<typename T> void wr(std::ofstream& f, const T& v)
        {
            f.write(reinterpret_cast<const char*>(&v), sizeof(T));
        }
        template<typename T> bool rd(std::ifstream& f, T& v)
        {
            return static_cast<bool>(f.read(reinterpret_cast<char*>(&v), sizeof(T)));
        }
    } //~ anonymous namespace

    bool binary_shader_storage::write_entry(std::ofstream& f, const shader_cache_entry& e)
    {
        wr(f, e.key);
        wr(f, e.schema_version);
        wr(f, e.source_hash);

        wr(f, static_cast<std::uint32_t>(e.identifier.size()));
        f.write(e.identifier.data(), static_cast<std::streamsize>(e.identifier.size()));

        wr(f, static_cast<std::uint32_t>(e.dxil.size()));
        f.write(reinterpret_cast<const char*>(e.dxil.data()),
                static_cast<std::streamsize>(e.dxil.size()));

        //~ the reflected vertex inputs
        wr(f, static_cast<std::uint32_t>(e.input_layout.size()));
        for (const auto& el : e.input_layout)
        {
            wr(f, static_cast<std::uint32_t>(el.semantic_name.size()));
            f.write(el.semantic_name.data(),
                    static_cast<std::streamsize>(el.semantic_name.size()));
            wr(f, el.semantic_index);
            wr(f, el.format);
            wr(f, el.input_slot);
        }

        //~ the depth pipeline this was baked for
        wr(f, e.depth_format);
        wr(f, e.depth_state);

        //~ the reflected bindings
        wr(f, static_cast<std::uint32_t>(e.bindings.size()));
        for (const auto& b : e.bindings)
        {
            wr(f, static_cast<std::uint32_t>(b.name.size()));
            f.write(b.name.data(),
                    static_cast<std::streamsize>(b.name.size()));
            wr(f, b.bind_point);
            wr(f, b.register_space);
            wr(f, b.bind_count);
            wr(f, b.type);
        }

        //~ the embedded root sig if any
        wr(f, static_cast<std::uint32_t>(e.embedded_root_sig.size()));
        f.write(reinterpret_cast<const char*>(e.embedded_root_sig.data()),
                static_cast<std::streamsize>(e.embedded_root_sig.size()));

        return static_cast<bool>(f);
    }

    bool binary_shader_storage::load_all(cache_map& out)
    {
        out.clear();
        std::ifstream f(path_, std::ios::binary);
        if (!f.is_open())
            return true;   //~ no file yet cold cache

        std::uint32_t magic = 0, version = 0;
        if (!rd(f, magic) || magic != k_magic)
        {
            LOG_WARN("[shader] bad cache magic");
            return true;
        }
        if (!rd(f, version) || version != k_version)
        {
            LOG_WARN("[shader] cache version mismatch");
            return true;
        }

        //~ the stored count is just a hint we read entries until EOF
        std::uint32_t hint = 0;
        rd(f, hint);

        while (f.peek() != EOF)
        {
            shader_cache_entry e{};
            std::uint32_t id_len = 0, dxil_len = 0, elem_count = 0;

            if (!rd(f, e.key) || !rd(f, e.schema_version)
                || !rd(f, e.source_hash) || !rd(f, id_len))
                break;
            if (id_len > k_max_string) { LOG_WARN("[shader] cache entry id too long"); break; }

            e.identifier.resize(id_len);
            f.read(e.identifier.data(), id_len);

            if (!rd(f, dxil_len))
                break;
            if (dxil_len > k_max_bytes) { LOG_WARN("[shader] cache dxil too big"); break; }
            e.dxil.resize(dxil_len);
            f.read(reinterpret_cast<char*>(e.dxil.data()), dxil_len);

            if (!rd(f, elem_count))
                break;
            if (elem_count > k_max_elements) { LOG_WARN("[shader] cache layout count wild"); break; }
            e.input_layout.reserve(elem_count);

            bool ok = true;
            for (std::uint32_t i = 0; i < elem_count; ++i)
            {
                vertex_input_element el{};
                std::uint32_t name_len = 0;
                if (!rd(f, name_len) || name_len > k_max_string)
                {
                    ok = false;
                    break;
                }

                el.semantic_name.resize(name_len);
                f.read(el.semantic_name.data(), name_len);

                if (!rd(f, el.semantic_index) ||
                    !rd(f, el.format) ||
                    !rd(f, el.input_slot))
                {
                    ok = false;
                    break;
                }
                e.input_layout.push_back(std::move(el));
            }
            if (!ok || !f) break;

            //~ the depth pipeline info
            if (!rd(f, e.depth_format) || !rd(f, e.depth_state))
                break;

            //~ the reflected bindings
            std::uint32_t bind_count = 0;
            if (!rd(f, bind_count))
                break;
            if (bind_count > k_max_elements) { LOG_WARN("[shader] cache binding count wild"); break; }
            e.bindings.reserve(bind_count);
            bool ok_bindings = true;
            for (std::uint32_t i = 0; i < bind_count; ++i)
            {
                reflected_binding b{};
                std::uint32_t name_len = 0;
                if (!rd(f, name_len) || name_len > k_max_string)
                {
                    ok_bindings = false;
                    break;
                }
                b.name.resize(name_len);
                f.read(b.name.data(), name_len);
                if (!rd(f, b.bind_point)     ||
                    !rd(f, b.register_space) ||
                    !rd(f, b.bind_count)     ||
                    !rd(f, b.type))
                {
                    ok_bindings = false;
                    break;
                }
                e.bindings.push_back(std::move(b));
            }
            if (!ok_bindings || !f) break;

            //~ the embedded root sig
            std::uint32_t rs_len = 0;
            if (!rd(f, rs_len))
                break;
            if (rs_len > k_max_bytes) { LOG_WARN("[shader] cache root sig too big"); break; }
            e.embedded_root_sig.resize(rs_len);
            if (rs_len > 0)
                f.read(reinterpret_cast<char*>(e.embedded_root_sig.data()), rs_len);

            out[e.key] = std::move(e);
        }
        LOG_INFO("[shader] loaded {} cached shader(s) (binary)", out.size());
        return true;
    }

    bool binary_shader_storage::store_all(const cache_map& in)
    {
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(path_).parent_path(), ec);

        std::ofstream f(path_, std::ios::binary | std::ios::trunc);
        if (!f.is_open()) { LOG_ERROR("[shader] cannot write {}", path_); return false; }

        wr(f, k_magic);
        wr(f, k_version);
        wr(f, static_cast<std::uint32_t>(in.size()));
        for (const auto& [key, e] : in)
            if (!write_entry(f, e))
                return false;

        LOG_INFO("[shader] compacted {} shader(s) -> {}", in.size(), path_);
        return true;
    }

    bool binary_shader_storage::ensure_header() const
    {
        if (std::filesystem::exists(path_))
            return true;

        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(path_).parent_path(), ec);

        std::ofstream f(path_, std::ios::binary | std::ios::trunc);
        if (!f.is_open()) return false;
        wr(f, k_magic);
        wr(f, k_version);
        wr(f, static_cast<std::uint32_t>(0));   //~ count hint zero load reads to EOF anyway
        return true;
    }

    bool binary_shader_storage::store_one(const shader_cache_entry& e, const cache_map&)
    {
        if (!ensure_header())
        {
            LOG_ERROR("[shader] cannot create {}", path_);
            return false;
        }

        std::ofstream f(path_, std::ios::binary | std::ios::app);
        if (!f.is_open())
        {
            LOG_ERROR("[shader] cannot append {}", path_);
            return false;
        }

        if (!write_entry(f, e))
        {
            LOG_ERROR("[shader] append failed");
            return false;
        }
        f.flush();
        return true;
    }
} // namespace trishul::render::shaders