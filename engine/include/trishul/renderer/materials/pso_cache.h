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
#ifndef CURSEOFTHESEA_PSO_CACHE_H
#define CURSEOFTHESEA_PSO_CACHE_H

#include <cstdint>
#include <vector>
#include <wrl/client.h>

#include "trishul/renderer/materials/material_registry.h"

struct ID3D12PipelineState;
struct ID3D12RootSignature;

namespace trishul::render::hardware
{
    class device;
    class deferred_releaser;
}
namespace trishul::render::shaders  { class shader_cache; }

namespace trishul::render::materials
{
    //~ the per shader pipeline state cache same get or create shape as the
    //~ shader cache one pso per shader id a shader that fails to build is
    //~ remembered as broken so we do not retry the bad compile on every draw
    //~
    //~ render thread only no locking
    class pso_cache final
    {
    public:
        pso_cache() = default;
        ~pso_cache();

        pso_cache           (const pso_cache&) = delete;
        pso_cache& operator=(const pso_cache&) = delete;

        [[nodiscard]] bool initialize(hardware::device&             device,
                                      shaders::shader_cache&        shaders,
                                      ID3D12RootSignature*          root_sig,
                                      hardware::deferred_releaser*  releaser);
        void               deinitialize();

        //~ handing back the pso for a shader id building it on a miss null on a
        //~ compile or pso failure the caller falls back to the default material
        [[nodiscard]] ID3D12PipelineState* get_or_create(shader_id id,
                                                         const shader_desc& desc);

        //~ dropping one entry the next get_or_create rebuilds it
        void invalidate(shader_id id);

        //~ dropping every entry rebuilding the whole world good for a shader
        //~ hot reload from the editor
        void invalidate_all();

        //~ stats handy for the cache hit assertions in tests
        [[nodiscard]] std::uint32_t hit_count()    const noexcept { return hits_;   }
        [[nodiscard]] std::uint32_t build_count()  const noexcept { return builds_; }
        [[nodiscard]] std::uint32_t broken_count() const noexcept { return broken_; }
        void                        reset_stats() noexcept;

        [[nodiscard]] ID3D12RootSignature* root_sig() const noexcept { return root_sig_; }

    private:
        bool build_entry(shader_id id, const shader_desc& desc);

        struct entry
        {
            Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
            bool                                        broken { false };
            bool                                        ready  { false };
        };

        hardware::device*             device_   { nullptr };
        shaders::shader_cache*        shaders_  { nullptr };
        ID3D12RootSignature*          root_sig_ { nullptr };
        hardware::deferred_releaser*  releaser_ { nullptr };

        std::vector<entry>      entries_;

        std::uint32_t hits_   { 0 };
        std::uint32_t builds_ { 0 };
        std::uint32_t broken_ { 0 };
    };
} // namespace trishul::render::materials

#endif //CURSEOFTHESEA_PSO_CACHE_H
