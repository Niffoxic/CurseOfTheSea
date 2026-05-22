// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_TRIANGLE_PASS_H
#define CURSEOFTHESEA_TRIANGLE_PASS_H

#include <cstdint>
#include <vector>
#include <wrl/client.h>

#include "engine/graphics/passes/pass.h"
#include "engine/graphics/hardware/resource.h"

struct ID3D12RootSignature;
struct ID3D12PipelineState;

namespace cots::graphics::passes
{
    //~ draws a hardcoded triangle and also hard coded PSO for now
    //  root sig is extracted from the shaders embedded
    class triangle_pass final : public pass
    {
    public:
        explicit triangle_pass(graph::resource_handle backbuffer) noexcept;

        bool setup  (const setup_context& sc)    override;
        void declare(graph::declare_context& dc) override;
        void execute(const pass_context& pc)     override;

        [[nodiscard]] const char* name() const noexcept override
        {
            return "triangle_pass";
        }

    private:
        //~ resolved per stream binding info
        struct vertex_stream
        {
            std::uint64_t gpu_address { 0 };
            std::uint32_t size_bytes  { 0 };
            std::uint32_t stride      { 0 };
        };

        graph::resource_handle backbuffer_;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> root_sig_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;

        std::vector<hardware::buffer_handle> vbufs_;
        std::vector<vertex_stream>           streams_; //~ following the stream from input slots
        std::uint32_t                        vertex_count_{ 3 };
    };
} // namespace cots::graphics::passes

#endif //CURSEOFTHESEA_TRIANGLE_PASS_H
