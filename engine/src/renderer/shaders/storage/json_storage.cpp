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
#include "trishul/renderer/shaders/storage/json_storage.h"

#include <nlohmann/json.hpp>
#include <filesystem>
#include <format>
#include <fstream>
#include <ranges>

#include "trishul/utils/logger.h"
#include "trishul/utils/statics.h"

namespace trishul::render::shaders
{
    bool json_shader_storage::load_all(cache_map& out)
    {
        out.clear();
        std::ifstream f(path_);

        if (!f.is_open()) //~ no file yet cold cache
            return true;

        nlohmann::json j;
        try
        {
            f >> j;
        }
        catch (const std::exception& e)
        {
            LOG_WARN("[shader] cache parse failed: {}", e.what());
            return true;
        }

        if (!j.contains("entries"))
            return true;

        for (const auto& je : j["entries"])
        {
            //~ one bad entry should not sink the whole load the hex parses can
            //~ throw so we catch per entry and just skip the rotten one
            try
            {
                shader_cache_entry e{};
                e.key = std::stoull(je.value("key", "0"), nullptr, 16);
                e.schema_version = je.value("schema_version", 0u);
                e.source_hash    = std::stoull(je.value("source_hash", "0"), nullptr, 16);
                e.identifier   = je.value("id", "");
                e.dxil         = statics::b64_decode(je.value("dxil", ""));
                e.depth_format = je.value("depth_format", 0u);
                e.depth_state  = je.value("depth_state",  0u);

                if (je.contains("layout"))
                {
                    for (const auto& jl : je["layout"])
                    {
                        vertex_input_element el{};
                        el.semantic_name  = jl.value("semantic", "");
                        el.semantic_index = jl.value("semantic_index", 0u);
                        el.format         = jl.value("format", 0u);
                        el.input_slot     = jl.value("input_slot", 0u);
                        e.input_layout.push_back(std::move(el));
                    }
                }

                if (je.contains("bindings"))
                {
                    for (const auto& jb : je["bindings"])
                    {
                        reflected_binding b{};
                        b.name           = jb.value("name", "");
                        b.bind_point     = jb.value("bind_point", 0u);
                        b.register_space = jb.value("space", 0u);
                        b.bind_count     = jb.value("count", 1u);
                        b.type           = jb.value("type", 0u);
                        e.bindings.push_back(std::move(b));
                    }
                }

                if (je.contains("root_sig"))
                {
                    e.embedded_root_sig = statics::b64_decode(je.value("root_sig", ""));
                }

                out.emplace(e.key, std::move(e));
            }
            catch (const std::exception& ex)
            {
                LOG_WARN("[shader] skipping bad cache entry: {}", ex.what());
            }
        }
        LOG_INFO("[shader] loaded {} cached shader(s) (json)", out.size());
        return true;
    }

    bool json_shader_storage::store_all(const cache_map& in)
    {
        std::error_code ec;
        std::filesystem::create_directories(
            std::filesystem::path(path_).parent_path(), ec);

        nlohmann::json j;
        j["version"] = 2;
        auto& arr = j["entries"] = nlohmann::json::array();
        for (const auto &e: in | std::views::values)
        {
            //~ packing the reflected layout
            nlohmann::json layout = nlohmann::json::array();
            for (const auto& el : e.input_layout)
                layout.push_back(
            {
                    { "semantic",       el.semantic_name  },
                    { "semantic_index", el.semantic_index },
                    { "format",         el.format         },
                    { "input_slot",     el.input_slot     },
                });

            //~ packing the reflected bindings
            nlohmann::json bindings = nlohmann::json::array();
            for (const auto& b : e.bindings)
                bindings.push_back(
            {
                    { "name",       b.name           },
                    { "bind_point", b.bind_point     },
                    { "space",      b.register_space },
                    { "count",      b.bind_count     },
                    { "type",       b.type           },
                });

            //~ the dxil and root sig ride as base64 the hashes as hex strings so
            //~ they survive the round trip and stay readable
            arr.push_back({
                { "key",            std::format("{:016x}", e.key)         },
                { "schema_version", e.schema_version                      },
                { "source_hash",    std::format("{:016x}", e.source_hash) },
                { "id",             e.identifier                          },
                { "dxil",           statics::b64_encode(e.dxil)           },
                { "layout",         layout                                },
                { "bindings",       bindings                              },
                { "root_sig",       statics::b64_encode(e.embedded_root_sig) },
                { "depth_format",   e.depth_format                        },
                { "depth_state",    e.depth_state                         },
            });
        }

        std::ofstream f(path_, std::ios::trunc);
        if (!f.is_open())
        {
            LOG_ERROR("[shader] cannot write {}", path_); return false;
        }
        f << j.dump(2);
        LOG_INFO("[shader] wrote {} shader(s) -> {}", in.size(), path_);
        return true;
    }
} // namespace trishul::render::shaders