// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_MESH_PASS_H
#define CURSEOFTHESEA_MESH_PASS_H

#include <cstdint>
#include <vector>
#include <wrl/client.h>

#include "engine/graphics/passes/pass.h"
#include "engine/graphics/resource/constant_ring.h"

struct ID3D12RootSignature;
struct ID3D12PipelineState;

namespace cots::graphics::passes
{
    //~ draws snapshot instances per frame
    class mesh_pass final : public pass
    {
    public:
        mesh_pass(graph::resource_handle backbuffer,
                  graph::resource_handle depth,
                  graph::resource_handle test_texture,
                  std::uint32_t          test_texture_index) noexcept;

        bool setup  (const setup_context& sc)    override;
        void declare(graph::declare_context& dc) override;
        void execute(const pass_context& pc)     override;

        [[nodiscard]]
        const char* name() const noexcept override
        {
            return "mesh_pass";
        }

    private:
        struct resolved_mesh
        {
            struct stream
            {
                std::uint64_t address;
                std::uint32_t size;
                std::uint32_t stride;
            };
            std::vector<stream> streams;

            std::uint32_t vertex_count  { 0 };

            bool          indexed       { false };
            std::uint64_t index_address { 0 };
            std::uint32_t index_size    { 0 };
            std::uint32_t index_count   { 0 };
            bool          index_16bit   { true };
        };

        graph::resource_handle backbuffer_;
        graph::resource_handle depth_;
        graph::resource_handle test_texture_;
        std::uint32_t          test_texture_index_ { 0u };

        Microsoft::WRL::ComPtr<ID3D12RootSignature> root_sig_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;

        std::vector<resolved_mesh> meshes_;     //~ indexed by mesh_id
        resource::constant_ring    frame_ring_;  //~ for view and projection
        resource::constant_ring    object_ring_; //~ whatever records per instance
    };
} // namespace cots::graphics::passes

#endif //CURSEOFTHESEA_MESH_PASS_H
