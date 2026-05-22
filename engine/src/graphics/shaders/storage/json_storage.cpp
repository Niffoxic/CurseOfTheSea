// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/shaders/storage/json_storage.h"

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

#include "engine/utils/helpers.h"

namespace cots::graphics::shaders
{
    bool json_shader_storage::load_all(cache_map& out)
    {
        out.clear();
        std::ifstream f(path_);

        if (!f.is_open()) //~ cold cache
            return true;

        nlohmann::json j;
        try
        {
            f >> j;
        }
        catch (const std::exception& e)
        {
            spdlog::warn("[shader] cache parse failed: {}", e.what());
            return true;
        }

        if (!j.contains("entries"))
            return true;

        for (const auto& je : j["entries"])
        {
            shader_cache_entry e{};
            e.key = std::stoull(
                je.value("key", "0"),
                nullptr,
                16
            );
            e.schema_version = je.value("schema_version", 0u);
            e.source_hash    = std::stoull(
                je.value("source_hash", "0"),
                nullptr,
                16
            );
            e.identifier   = je.value("id", "");
            e.dxil         = helpers::b64_decode(je.value("dxil", ""));
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
            out.emplace(e.key, std::move(e));
        }
        spdlog::info("[shader] loaded {} cached shader(s) (json)", out.size());
        return true;
    }

    bool json_shader_storage::store_all(const cache_map& in)
    {
        std::error_code ec;
        std::filesystem::create_directories(
            std::filesystem::path(path_).parent_path(), ec);

        nlohmann::json j;
        j["version"] = 1;
        auto& arr = j["entries"] = nlohmann::json::array();
        for (const auto &e: in | std::views::values)
        {
            nlohmann::json layout = nlohmann::json::array();
            for (const auto& el : e.input_layout)
                layout.push_back(
            {
                    { "semantic",       el.semantic_name  },
                    { "semantic_index", el.semantic_index },
                    { "format",         el.format         },
                    { "input_slot",     el.input_slot     },
                });

            arr.push_back({
                { "key",            std::format("{:016x}", e.key)         },
                { "schema_version", e.schema_version                      },
                { "source_hash",    std::format("{:016x}", e.source_hash) },
                { "id",             e.identifier                          },
                { "dxil",           helpers::b64_encode(e.dxil)           },
                { "layout",         layout                                },
                { "depth_format",   e.depth_format                        },
                { "depth_state",    e.depth_state                         },
            });
        }

        std::ofstream f(path_, std::ios::trunc);
        if (!f.is_open())
        {
            spdlog::error("[shader] cannot write {}", path_); return false;
        }
        f << j.dump(2);
        spdlog::info("[shader] wrote {} shader(s) -> {}", in.size(), path_);
        return true;
    }
} // namespace cots::graphics::shaders
