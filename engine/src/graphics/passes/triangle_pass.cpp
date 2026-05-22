// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/passes/triangle_pass.h"
#include "engine/graphics/hardware/device.h"
#include "engine/graphics/hardware/buffer_manager.h"
#include "engine/graphics/shaders/shader_cache.h"

#include <d3d12.h>
#include <algorithm>
#include <spdlog/spdlog.h>

namespace cots::graphics::passes
{
    namespace
    {
        //~ hardcoded triangle for test only
        constexpr float k_positions[] =
        {
             0.0f,  0.5f, 0.0f,
             0.5f, -0.5f, 0.0f,
            -0.5f, -0.5f, 0.0f,
        };
        constexpr float k_colors[] =
        {
            1.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 1.0f,
        };

        struct attrib_source
        {
            const void*   data;
            std::uint32_t bytes;
            std::uint32_t stride;
        };

        //~ reflection gives us the saxy semantic
        attrib_source source_for(const std::string& semantic)
        {
            if (semantic == "POSITION")
                return {
                    k_positions,
                    sizeof(k_positions),
                    sizeof(float) * 3
                };
            if (semantic == "COLOR")
                return {
                    k_colors,
                    sizeof(k_colors),
                    sizeof(float) * 3
                };
            return {
                nullptr,
                0,
                0
            };
        }
    } // namespace anonymous

    bool triangle_pass::setup(const setup_context& sc)
    {
        auto* d3d = sc.device.d3d12_device();
        if (!d3d)
        {
            spdlog::error("[triangle] no device");
            return false;
        }

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

        //~ root signature embedded directly from byte
        if (FAILED(d3d->CreateRootSignature(0, vs.data, vs.size, IID_PPV_ARGS(&root_sig_))))
        {
            spdlog::error("[triangle] CreateRootSignature from embedded sig failed");
            return false;
        }
        (void)root_sig_->SetName(L"triangle_pass root sig");

        //~ building the input layout from reflection
        //  semantic name points into the cache entry strings which outlive this
        //  PSO so the pointers stay valid
        std::vector<D3D12_INPUT_ELEMENT_DESC> elems;
        if (vs.input_layout && !vs.input_layout->empty())
        {
            elems.reserve(vs.input_layout->size());
            streams_.assign(vs.input_layout->size(), vertex_stream{});
            vbufs_.reserve(vs.input_layout->size());

            for (const auto& el : *vs.input_layout)
            {
                D3D12_INPUT_ELEMENT_DESC d{};
                d.SemanticName         = el.semantic_name.c_str();
                d.SemanticIndex        = el.semantic_index;
                d.Format               = static_cast<DXGI_FORMAT>(el.format);
                d.InputSlot            = el.input_slot;
                d.AlignedByteOffset    = 0;
                d.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
                d.InstanceDataStepRate = 0;
                elems.push_back(d);

                const attrib_source src = source_for(el.semantic_name);
                if (!src.data)
                {
                    spdlog::error("[triangle] no data source for semantic '{}'",
                                  el.semantic_name);
                    return false;
                }

                hardware::buffer_create_info bi{};
                bi.size_bytes   = src.bytes;
                bi.kind         = hardware::buffer_kind::vertex;
                bi.initial_data = src.data;
                bi.stride       = src.stride;
                bi.debug_name   = el.semantic_name.c_str();

                const auto h = sc.buffers.create(bi);
                if (!h.valid())
                {
                    spdlog::error("[triangle] vertex buffer create failed for '{}'",
                                  el.semantic_name);
                    return false;
                }
                vbufs_.push_back(h);

                if (el.input_slot >= streams_.size())
                    streams_.resize(el.input_slot + 1);

                streams_[el.input_slot] = vertex_stream
                {
                    sc.buffers.gpu_address(h),
                    static_cast<std::uint32_t>(sc.buffers.size(h)),
                    static_cast<std::uint32_t>(sc.buffers.stride(h)),
                };
            }

            if (!streams_.empty() && streams_[0].stride)
                vertex_count_ = streams_[0].size_bytes / streams_[0].stride;
        }
        else
        {
            spdlog::warn("[triangle] VS reported no input layout");
            return false;
        }

        //~ TODO: gotta code builder sometimes later
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
        pso.pRootSignature = root_sig_.Get();
        pso.VS             = { vs.data, vs.size };
        pso.PS             = { ps.data, ps.size };
        pso.InputLayout    = { elems.data(), static_cast<UINT>(elems.size()) };

        pso.RasterizerState.FillMode              = D3D12_FILL_MODE_SOLID;
        pso.RasterizerState.CullMode              = D3D12_CULL_MODE_NONE;
        pso.RasterizerState.FrontCounterClockwise = FALSE;
        pso.RasterizerState.DepthClipEnable       = TRUE;

        pso.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

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

        spdlog::info("[triangle] pass ready (vs={} ps={} bytes, {} stream(s), {} verts)",
                     vs.size, ps.size, streams_.size(), vertex_count_);
        return true;
    }

    void triangle_pass::execute(const pass_context& pc)
    {
        auto* list = pc.ctx.list();

        pc.ctx.set_render_target(pc.rtv_handle);

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

        //~ one vertex stream per attribute on stack tho
        if (!streams_.empty())
        {
            constexpr std::size_t k_max = 8;
            D3D12_VERTEX_BUFFER_VIEW views[k_max]{};
            const UINT n = static_cast<UINT>(
                std::min(streams_.size(), k_max));

            for (UINT i = 0; i < n; ++i)
            {
                views[i].BufferLocation = streams_[i].gpu_address;
                views[i].SizeInBytes    = streams_[i].size_bytes;
                views[i].StrideInBytes  = streams_[i].stride;
            }
            list->IASetVertexBuffers(0, n, views);
        }

        list->DrawInstanced(
            vertex_count_,
            1, 0,
            0
        );
    }
} // namespace cots::graphics::passes
