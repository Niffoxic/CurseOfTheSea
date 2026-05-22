// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/passes/mesh_pass.h"
#include "engine/graphics/hardware/device.h"
#include "engine/graphics/hardware/buffer_manager.h"
#include "engine/graphics/meshes/mesh_registry.h"
#include "engine/graphics/shaders/shader_cache.h"
#include "engine/utils/profiler.h"

#include <d3d12.h>
#include <DirectXMath.h>
#include <algorithm>
#include <spdlog/spdlog.h>

namespace cots::graphics::passes
{
    namespace //~ mesh specific locals
    {
        constexpr std::uint32_t k_max_instances = 1024;

        //~ constant buffers
        struct frame_constants
        {
            DirectX::XMFLOAT4X4 view_proj;
        };
        struct object_constants
        {
            DirectX::XMFLOAT4X4 world;
            std::uint32_t       material;
            std::uint32_t       pad[3];
        };
    } //~ anonymouse namespace

    bool mesh_pass::setup(const setup_context& sc)
    {
        auto* d3d = sc.device.d3d12_device();
        if (!d3d)
        {
            spdlog::error("[mesh] no device");
            return false;
        }

        const auto vs = sc.shaders.get_or_compile(
            "assets/shaders/mesh.hlsl",
            "VSMain",
            shaders::shader_stage::vertex
        );
        const auto ps = sc.shaders.get_or_compile(
            "assets/shaders/mesh.hlsl",
            "PSMain",
            shaders::shader_stage::pixel
        );

        if (!vs.valid() || !ps.valid() || !vs.input_layout || vs.input_layout->empty())
        {
            spdlog::error("[mesh] shader compile/cache/layout failed");
            return false;
        }

        if (FAILED(d3d->CreateRootSignature(0, vs.data, vs.size, IID_PPV_ARGS(&root_sig_))))
        {
            spdlog::error("[mesh] CreateRootSignature failed");
            return false;
        }
        (void)root_sig_->SetName(L"mesh_pass root sig");

        //~ input layout from reflection
        std::vector<D3D12_INPUT_ELEMENT_DESC> elems;
        elems.reserve(vs.input_layout->size());
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
        }

        //~ resolving every registered mesh against the reflected layout
        meshes_.clear();
        meshes_.reserve(sc.meshes.size());
        for (std::uint32_t id = 0; id < sc.meshes.size(); ++id)
        {
            const auto* me = sc.meshes.get(id);
            resolved_mesh rm{};
            rm.vertex_count = me->vertex_count;
            rm.streams.reserve(vs.input_layout->size());

            for (const auto& el : *vs.input_layout)
            {
                const auto* st = me->find(el.semantic_name);
                if (!st)
                {
                    spdlog::error("[mesh] mesh {} missing stream '{}'", id, el.semantic_name);
                    return false;
                }
                rm.streams.push_back(
        {
                    sc.buffers.gpu_address(st->buffer),
                    static_cast<std::uint32_t>(sc.buffers.size(st->buffer)),
                    st->stride,
                });
            }

            if (me->index.valid())
            {
                rm.indexed       = true;
                rm.index_address = sc.buffers.gpu_address(me->index);
                rm.index_size    = static_cast<std::uint32_t>(sc.buffers.size(me->index));
                rm.index_count   = me->index_count;
                rm.index_16bit   = me->index_16bit;
            }
            meshes_.push_back(std::move(rm));
        }

        //~ per frame constant rings
        if (!frame_ring_ .initialize(sc.buffers,
            sizeof(frame_constants),
            1, "mesh_frame_cb"))
        {
            return false;
        }
        if (!object_ring_.initialize(sc.buffers,
            sizeof(object_constants),
            k_max_instances, "mesh_object_cb"))
        {
            return false;
        }

        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
        pso.pRootSignature = root_sig_.Get();
        pso.VS             = { vs.data, vs.size };
        pso.PS             = { ps.data, ps.size };
        pso.InputLayout    = { elems.data(), static_cast<UINT>(elems.size()) };

        pso.RasterizerState.FillMode              = D3D12_FILL_MODE_SOLID;
        pso.RasterizerState.CullMode              = D3D12_CULL_MODE_NONE; //~ testing with flat right now
        pso.RasterizerState.FrontCounterClockwise = FALSE;
        pso.RasterizerState.DepthClipEnable       = TRUE;

        pso.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        //~ reversed z for better comparison depth write on stencil reserved (disabled for now)
        pso.DepthStencilState.DepthEnable    = TRUE;
        pso.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        pso.DepthStencilState.DepthFunc      = D3D12_COMPARISON_FUNC_GREATER;
        pso.DepthStencilState.StencilEnable  = FALSE;

        pso.SampleMask            = UINT_MAX;
        pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pso.NumRenderTargets      = 1;
        pso.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM;
        pso.DSVFormat             = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        pso.SampleDesc            = { 1, 0 };

        if (FAILED(d3d->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&pso_))))
        {
            spdlog::error("[mesh] CreateGraphicsPipelineState failed");
            return false;
        }
        (void)pso_->SetName(L"mesh_pass PSO");

        spdlog::info("[mesh] ready ({} mesh(es), cap {} instances)", meshes_.size(), k_max_instances);
        return true;
    }

    void mesh_pass::execute(const pass_context& pc)
    {
        COTS_PROFILE_SCOPE("mesh_pass::execute");

        using namespace DirectX;
        auto* list            = pc.ctx.list();
        const std::uint32_t f = pc.frame_index;

        //~ frame cb row major
        const XMMATRIX view = XMLoadFloat4x4(&pc.snap.camera.view);
        const XMMATRIX proj = XMLoadFloat4x4(&pc.snap.camera.projection);

        frame_constants fc{};
        XMStoreFloat4x4(&fc.view_proj, XMMatrixMultiply(view, proj));
        if (void* dst = frame_ring_.cpu(f, 0))
        {
            std::memcpy(dst, &fc, sizeof(fc));
        }

        pc.ctx.set_render_target(pc.rtv_handle, pc.dsv_handle);

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

        //~ b0 set one for all (wow one for all xD what was it? my hero academia xD)
        list->SetGraphicsRootConstantBufferView(0, frame_ring_.gpu(f, 0));

        std::uint32_t drawn     = 0;
        std::uint32_t last_id   = ~0u;   //~ rebind streams only on mesh change
        const std::uint32_t cap = object_ring_.count();

        for (const auto& inst : pc.snap.instances)
        {
            if (drawn >= cap)
            {
                spdlog::warn("[mesh] instance cap {} hit", cap);
                break;
            }
            if (inst.mesh_index >= meshes_.size())
                continue;

            const resolved_mesh& rm = meshes_[inst.mesh_index];

            if (inst.mesh_index != last_id)
            {
                D3D12_VERTEX_BUFFER_VIEW views[8]{};
                const UINT n = static_cast<UINT>(std::min<std::size_t>(rm.streams.size(), 8));
                for (UINT s = 0; s < n; ++s)
                {
                    views[s].BufferLocation = rm.streams[s].address;
                    views[s].SizeInBytes    = rm.streams[s].size;
                    views[s].StrideInBytes  = rm.streams[s].stride;
                }
                list->IASetVertexBuffers(0, n, views);

                if (rm.indexed)
                {
                    D3D12_INDEX_BUFFER_VIEW ibv{};
                    ibv.BufferLocation = rm.index_address;
                    ibv.SizeInBytes    = rm.index_size;
                    ibv.Format         = rm.index_16bit ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
                    list->IASetIndexBuffer(&ibv);
                }
                last_id = inst.mesh_index;
            }

            //~ per object cb into this frames ring slot
            object_constants oc{};
            oc.world    = inst.transform;
            oc.material = inst.material_index;

            if (void* dst = object_ring_.cpu(f, drawn))
            {
                std::memcpy(dst, &oc, sizeof(oc));
            }
            list->SetGraphicsRootConstantBufferView(
                1,
                object_ring_.gpu(f, drawn)
            );

            if (rm.indexed)
            {
                list->DrawIndexedInstanced(
                    rm.index_count,
                    1,
                    0,
                    0,
                    0
                );
            }
            else
            {
                list->DrawInstanced(
                    rm.vertex_count,
                    1,
                    0,
                    0
                );
            }
            ++drawn;
        }
    }
} // namespace cots::graphics::passes
