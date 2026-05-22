// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/passes/triangle_pass.h"
#include "engine/graphics/hardware/device.h"
#include "engine/graphics/shaders/shader_cache.h"

#include <d3d12.h>
#include <spdlog/spdlog.h>

namespace cots::graphics::passes
{
    bool triangle_pass::setup(const setup_context& sc)
    {
        auto* d3d = sc.device.d3d12_device();
        if (!d3d)
        {
            spdlog::error("[triangle] no device");
            return false;
        }

        //~ compile shaders
        const auto vs = sc.shaders.get_or_compile(
            "assets/shaders/triangle.hlsl",
            "VSMain",
            shaders::shader_stage::vertex
        );
        const auto ps = sc.shaders.get_or_compile(
            "assets/shaders/triangle.hlsl",
            "PSMain",
            shaders::shader_stage::pixel
        );

        if (!vs.valid() || !ps.valid())
        {
            spdlog::error("[triangle] shader compile/cache failed");
            return false;
        }

        //~ extract the root signature embedded in the VS bytecode
        if (FAILED(d3d->CreateRootSignature(0, vs.data, vs.size,
            IID_PPV_ARGS(&root_sig_))))
        {
            spdlog::error("[triangle] CreateRootSignature from embedded sig failed");
            return false;
        }
        (void)root_sig_->SetName(L"triangle_pass root sig");

        //~ TODO: remove hard coded PSO with reflection or a builder
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
        //~ shader related
        pso.pRootSignature = root_sig_.Get();
        pso.VS          = { vs.data, vs.size };
        pso.PS          = { ps.data, ps.size };
        pso.InputLayout = { nullptr, 0 };

        //~ rasterizer related
        pso.RasterizerState.FillMode              = D3D12_FILL_MODE_SOLID;
        pso.RasterizerState.CullMode              = D3D12_CULL_MODE_NONE;
        pso.RasterizerState.FrontCounterClockwise = FALSE;
        pso.RasterizerState.DepthClipEnable       = TRUE;

        //~ defaults
        pso.BlendState.RenderTarget[0].RenderTargetWriteMask =
            D3D12_COLOR_WRITE_ENABLE_ALL;

        pso.DepthStencilState.DepthEnable   = FALSE;
        pso.DepthStencilState.StencilEnable = FALSE;

        pso.SampleMask            = UINT_MAX;
        pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pso.NumRenderTargets      = 1;
        pso.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM;
        pso.SampleDesc            = { 1, 0 };

        if (FAILED(d3d->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&pso_))))
        {
            spdlog::error("[triangle] CreateGraphicsPipelineState failed");
            return false;
        }
        (void)pso_->SetName(L"triangle_pass PSO");

        spdlog::info("[triangle] pass ready (vs={} ps={} bytes)", vs.size, ps.size);
        return true;
    }

    void triangle_pass::execute(const pass_context& pc)
    {
        auto* list = pc.ctx.list();

        pc.ctx.set_render_target(pc.rtv_handle);

        //~ viewport and scissor
        const D3D12_VIEWPORT vp
        {
            0.0f, 0.0f,
            static_cast<float>(pc.width),
            static_cast<float>(pc.height),
            0.0f, 1.0f
        };
        const D3D12_RECT scissor
        {
            0, 0,
            static_cast<LONG>(pc.width),
            static_cast<LONG>(pc.height)
        };
        list->RSSetViewports(1, &vp);
        list->RSSetScissorRects(1, &scissor);

        list->SetGraphicsRootSignature(root_sig_.Get());
        list->SetPipelineState(pso_.Get());
        list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        list->DrawInstanced(3, 1,
            0, 0
        );
    }
} // namespace cots::graphics::passes
