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
#include "trishul/renderer/materials/pso_cache.h"

#include "trishul/renderer/hardware/device.h"
#include "trishul/renderer/hardware/deferred_releaser.h"
#include "trishul/renderer/shaders/shader_cache.h"
#include "trishul/renderer/materials/material_layout.h"
#include "trishul/core/engine_config.h"
#include "trishul/utils/logger.h"
#include "trishul/utils/statics.h"

#include <array>
#include <d3d12.h>

namespace trishul::render::materials
{
    pso_cache::~pso_cache() { deinitialize(); }

    bool pso_cache::initialize(hardware::device& device,
                               shaders::shader_cache& shaders,
                               ID3D12RootSignature* root_sig,
                               hardware::deferred_releaser* releaser)
    {
        if (!root_sig)
        {
            LOG_ERROR("[pso-cache] init missing root sig");
            return false;
        }
        device_   = &device;
        shaders_  = &shaders;
        root_sig_ = root_sig;
        releaser_ = releaser;
        entries_.clear();
        hits_ = builds_ = broken_ = 0u;
        return true;
    }

    void pso_cache::deinitialize()
    {
        //~ shutdown is post gpu idle so freeing inline is fine no releaser dance
        entries_.clear();
        device_   = nullptr;
        shaders_  = nullptr;
        root_sig_ = nullptr;
        releaser_ = nullptr;
    }

    void pso_cache::reset_stats() noexcept
    {
        hits_ = builds_ = broken_ = 0u;
    }

    ID3D12PipelineState* pso_cache::get_or_create(const shader_id id,
                                                  const shader_desc& desc)
    {
        if (id < 0) return nullptr;
        const auto idx = static_cast<std::uint32_t>(id);
        if (idx >= entries_.size()) entries_.resize(idx + 1u);

        auto& e = entries_[idx];

        if (e.ready)
        {
            //~ ready already whether good or broken counts as a hit we never
            //~ retry a known bad compile on the draw path
            ++hits_;
            return e.broken ? nullptr : e.pso.Get();
        }

        if (!build_entry(id, desc))
        {
            return nullptr;
        }
        return e.broken ? nullptr : e.pso.Get();
    }

    void pso_cache::invalidate(const shader_id id)
    {
        if (id < 0) return;
        const auto idx = static_cast<std::uint32_t>(id);
        if (idx >= entries_.size()) return;
        //~ deferring the old pso the gpu may still be drawing with it
        if (releaser_ && entries_[idx].pso)
        {
            releaser_->enqueue_com(std::move(entries_[idx].pso));
        }
        entries_[idx] = {};
        LOG_INFO("[pso-cache] invalidated shader id {}", id);
    }

    void pso_cache::invalidate_all()
    {
        //~ deferring every live pso any of them could still be in flight
        if (releaser_)
        {
            for (auto& e : entries_)
            {
                if (e.pso) releaser_->enqueue_com(std::move(e.pso));
            }
        }
        const std::size_t n = entries_.size();
        for (auto& e : entries_) e = {};
        LOG_INFO("[pso-cache] invalidated all {} entries", n);
    }

    bool pso_cache::build_entry(const shader_id id, const shader_desc& desc)
    {
        const auto idx = static_cast<std::uint32_t>(id);
        auto& e = entries_[idx];
        e = {};

        if (!device_ || !shaders_ || !root_sig_)
        {
            LOG_ERROR("[pso-cache] build called before init");
            e.broken = true;
            e.ready  = true;
            ++broken_;
            return false;
        }

        const auto vs = shaders_->get_or_compile(desc.path,
                                                 desc.vs_entry,
                                                 shaders::shader_stage::vertex);
        const auto ps = shaders_->get_or_compile(desc.path,
                                                 desc.ps_entry,
                                                 shaders::shader_stage::pixel);
        if (!vs.valid() || !ps.valid())
        {
            LOG_ERROR("[pso-cache] shader compile failed for id {} path {}",
                      id, desc.path);
            e.broken = true;
            e.ready  = true;
            ++broken_;
            return false;
        }

        //~ always feeding the full canonical layout a shader that only consumes
        //~ a subset is fine the input assembler ignores the extra streams
        std::array<D3D12_INPUT_ELEMENT_DESC, k_canonical_vertex_attrs.size()> elems{};
        for (std::size_t i = 0; i < k_canonical_vertex_attrs.size(); ++i)
        {
            const auto& a = k_canonical_vertex_attrs[i];
            elems[i].SemanticName         = a.semantic_name;
            elems[i].SemanticIndex        = a.semantic_index;
            elems[i].Format               = a.format;
            elems[i].InputSlot            = a.input_slot;
            elems[i].AlignedByteOffset    = 0u;
            elems[i].InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            elems[i].InstanceDataStepRate = 0u;
        }

        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
        pso.pRootSignature = root_sig_;
        pso.VS             = { vs.data, vs.size };
        pso.PS             = { ps.data, ps.size };
        pso.InputLayout    = { elems.data(), static_cast<UINT>(elems.size()) };

        pso.RasterizerState.FillMode              = D3D12_FILL_MODE_SOLID;
        pso.RasterizerState.CullMode              = D3D12_CULL_MODE_NONE;
        pso.RasterizerState.FrontCounterClockwise = FALSE;
        pso.RasterizerState.DepthClipEnable       = TRUE;

        pso.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        //~ reversed z depth greater test this has to line up with the shader
        //~ cache depth contract in engine_config or the cache would say hit while
        //~ the pso draws against a different depth setup
        pso.DepthStencilState.DepthEnable    = TRUE;
        pso.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        pso.DepthStencilState.DepthFunc      = D3D12_COMPARISON_FUNC_GREATER;
        pso.DepthStencilState.StencilEnable  = FALSE;

        pso.SampleMask            = UINT_MAX;
        pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pso.NumRenderTargets      = 1u;
        //~ forward passes write the hdr float scene target tonemap brings it down
        //~ to the ldr backbuffer later
        pso.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT;
        pso.DSVFormat             = static_cast<DXGI_FORMAT>(config::DEPTH_FORMAT);
        pso.SampleDesc            = { 1u, 0u };

        if (FAILED(device_->d3d12_device()->CreateGraphicsPipelineState(
                       &pso, IID_PPV_ARGS(&e.pso))))
        {
            LOG_ERROR("[pso-cache] CreateGraphicsPipelineState failed for id {} path {}",
                      id, desc.path);
            e.pso.Reset();
            e.broken = true;
            e.ready  = true;
            ++broken_;
            return false;
        }

        //~ naming the pso after the shader so dred and pix can point a finger
        {
            std::string tag = "material pso ";
            tag.append(desc.path);
            tag.append(" ");
            tag.append(desc.vs_entry);
            tag.append("/");
            tag.append(desc.ps_entry);
            const std::wstring wname = statics::to_wide(tag);
            (void)e.pso->SetName(wname.c_str());
        }

        e.broken = false;
        e.ready  = true;
        ++builds_;
        LOG_INFO("[pso-cache] built pso for shader id {} path {}", id, desc.path);
        return true;
    }
} // namespace trishul::render::materials
