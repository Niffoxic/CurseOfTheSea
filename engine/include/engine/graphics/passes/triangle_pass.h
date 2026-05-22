// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_TRIANGLE_PASS_H
#define CURSEOFTHESEA_TRIANGLE_PASS_H

#include <wrl/client.h>

#include "engine/graphics/passes/pass.h"

struct ID3D12RootSignature;
struct ID3D12PipelineState;

namespace cots::graphics::passes
{
    //~ draws a hardcoded triangle and also hard coded PSO for now
    //  root sig is extracted from the shaders embedded
    class triangle_pass final : public pass
    {
    public:
        bool setup  (const setup_context& sc) override;
        void execute(const pass_context& pc)  override;

        [[nodiscard]] const char* name() const noexcept override
        {
            return "triangle_pass";
        }

    private:
        Microsoft::WRL::ComPtr<ID3D12RootSignature> root_sig_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;
    };
} // namespace cots::graphics::passes

#endif //CURSEOFTHESEA_TRIANGLE_PASS_H
