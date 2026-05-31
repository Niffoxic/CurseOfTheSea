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
#include "trishul/renderer/particles/particle_system.h"

#include "trishul/renderer/hardware/device.h"
#include "trishul/renderer/hardware/buffer_manager.h"
#include "trishul/renderer/hardware/deferred_releaser.h"
#include "trishul/renderer/hardware/fence.h"
#include "trishul/renderer/shaders/shader_cache.h"
#include "trishul/renderer/shaders/root_sig_builder.h"
#include "trishul/renderer/render_snapshot.h"
#include "trishul/core/engine_config.h"
#include "trishul/utils/logger.h"

#include <d3d12.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>

namespace trishul::render::particles
{
    namespace
    {
        //~ root parameter slots mirroring the embedded rs strings in the matching
        //~ hlsl any edit to a shader rs string has to update the slot used here
        constexpr std::uint32_t k_rp_init_cb      = 0u;
        constexpr std::uint32_t k_rp_init_pool    = 1u;
        constexpr std::uint32_t k_rp_init_dead    = 2u;
        constexpr std::uint32_t k_rp_init_counter = 3u;

        constexpr std::uint64_t k_pool_size_bytes =
            static_cast<std::uint64_t>(k_max_particles) * sizeof(particle);
        constexpr std::uint64_t k_dead_size_bytes =
            static_cast<std::uint64_t>(k_max_particles) * sizeof(std::uint32_t);
        constexpr std::uint64_t k_alive_size_bytes =
            static_cast<std::uint64_t>(k_max_particles) * sizeof(std::uint32_t);
        constexpr std::uint64_t k_counter_size_bytes =
            sizeof(counter_layout);
        constexpr std::uint64_t k_sim_args_size_bytes =
            sizeof(dispatch_args_layout);
        constexpr std::uint64_t k_draw_args_size_bytes =
            sizeof(draw_args_layout);
        constexpr std::uint64_t k_emitter_ring_size_bytes =
            static_cast<std::uint64_t>(k_max_emitters_per_frame) * sizeof(emitter_gpu);

        struct init_cb_t
        {
            std::uint32_t max_particles;
        };

        //~ building a compute pso from a shader path a missing optional shader
        //~ short circuits to a clean null so a phase one build does not spew
        //~ errors for the phase two and three shaders that land later the caller
        //~ checks the com ptr null to decide whether the pipeline is usable
        bool build_compute_pso(hardware::device&        dev,
                               shaders::shader_cache&   shaders,
                               const char*              path,
                               const char*              entry,
                               Microsoft::WRL::ComPtr<ID3D12RootSignature>& rs,
                               Microsoft::WRL::ComPtr<ID3D12PipelineState>& pso,
                               const wchar_t*           debug_name,
                               const bool               optional)
        {
            //~ an optional shader that is not on disk yet just bows out quietly
            if (optional && !std::filesystem::exists(path))
            {
                LOG_INFO("[particles] optional shader not present yet: {}", path);
                rs.Reset();
                pso.Reset();
                return false;
            }

            const auto cs = shaders.get_or_compile(
                path, entry, shaders::shader_stage::compute);
            if (!cs.valid())
            {
                LOG_WARN("[particles] shader compile failed: {} {} pipeline disabled",
                         path, entry);
                rs.Reset();
                pso.Reset();
                return false;
            }

            const shaders::shader_bytecode stages[] = { cs };
            std::vector<std::uint8_t> rs_blob;
            if (!shaders::build_program_root_sig(stages, rs_blob) || rs_blob.empty())
            {
                LOG_ERROR("[particles] root sig build failed: {} {}", path, entry);
                return false;
            }

            auto* d3d = dev.d3d12_device();
            Microsoft::WRL::ComPtr<ID3D12RootSignature> new_rs;
            if (FAILED(d3d->CreateRootSignature(0, rs_blob.data(), rs_blob.size(),
                                                IID_PPV_ARGS(&new_rs))))
            {
                LOG_ERROR("[particles] CreateRootSignature failed: {} {}", path, entry);
                return false;
            }

            D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};
            desc.pRootSignature = new_rs.Get();
            desc.CS             = { cs.data, cs.size };

            Microsoft::WRL::ComPtr<ID3D12PipelineState> new_pso;
            if (FAILED(d3d->CreateComputePipelineState(&desc, IID_PPV_ARGS(&new_pso))))
            {
                LOG_ERROR("[particles] CreateComputePipelineState failed: {} {}", path, entry);
                return false;
            }
            new_pso->SetName(debug_name);

            rs  = std::move(new_rs);
            pso = std::move(new_pso);
            return true;
        }
    } //~ anonymous namespace

    bool particle_system::initialize(hardware::device&         dev,
                                     hardware::buffer_manager& bm,
                                     shaders::shader_cache&    shaders,
                                     ID3D12CommandQueue*       graphics_queue)
    {
        if (!dev.d3d12_device())
        {
            LOG_ERROR("[particles] no device");
            return false;
        }
        if (!graphics_queue)
        {
            LOG_ERROR("[particles] no graphics queue");
            return false;
        }

        device_  = &dev;
        buffers_ = &bm;
        shaders_ = &shaders;

        if (!allocate_buffers(bm)) return false;
        if (!build_psos())         return false;
        if (!build_command_signatures()) return false;
        if (!run_init_compute(graphics_queue)) return false;

        //~ dropping the init pso pair we never dispatch it again past here the
        //~ releaser is not wired this early in boot so an inline reset is fine
        //~ the gpu already waited inside run_init_compute
        init_pso_.Reset();
        init_rs_ .Reset();

        ready_ = true;
        LOG_INFO("[particles] system ready max_particles={} pool_bytes={} "
                 "alive ping pong + dead + counter seeded",
                 k_max_particles, k_pool_size_bytes);
        return true;
    }

    void particle_system::deinitialize() noexcept
    {
        if (buffers_)
        {
            if (pool_     .valid()) buffers_->destroy(pool_);
            if (dead_     .valid()) buffers_->destroy(dead_);
            for (auto& h : alive_)
            {
                if (h.valid()) buffers_->destroy(h);
                h = {};
            }
            if (counter_  .valid()) buffers_->destroy(counter_);
            if (sim_args_ .valid()) buffers_->destroy(sim_args_);
            if (draw_args_.valid()) buffers_->destroy(draw_args_);
            for (auto& h : emitter_ring_)
            {
                if (h.valid()) buffers_->destroy(h);
                h = {};
            }
        }
        pool_      = {};
        dead_      = {};
        counter_   = {};
        sim_args_  = {};
        draw_args_ = {};

        simulate_pso_.Reset();
        simulate_rs_ .Reset();
        emit_pso_    .Reset();
        emit_rs_     .Reset();
        reset_pso_   .Reset();
        reset_rs_    .Reset();
        sim_args_pso_.Reset();
        sim_args_rs_ .Reset();
        draw_args_pso_.Reset();
        draw_args_rs_.Reset();
        init_pso_    .Reset();
        init_rs_     .Reset();
        dispatch_sig_.Reset();
        draw_sig_    .Reset();

        ready_ = false;
        device_  = nullptr;
        buffers_ = nullptr;
        shaders_ = nullptr;
    }

    bool particle_system::allocate_buffers(hardware::buffer_manager& bm)
    {
        //~ the pool every particle record lives here default heap uav
        hardware::buffer_create_info bi_pool{};
        bi_pool.size_bytes = k_pool_size_bytes;
        bi_pool.kind       = hardware::buffer_kind::default_uav;
        bi_pool.stride     = sizeof(particle);
        bi_pool.debug_name = "particle_pool";
        pool_ = bm.create(bi_pool);
        if (!pool_.valid())
        {
            LOG_ERROR("[particles] pool alloc failed");
            return false;
        }

        //~ the dead list free slot indices default heap uav
        hardware::buffer_create_info bi_dead{};
        bi_dead.size_bytes = k_dead_size_bytes;
        bi_dead.kind       = hardware::buffer_kind::default_uav;
        bi_dead.stride     = sizeof(std::uint32_t);
        bi_dead.debug_name = "particle_dead";
        dead_ = bm.create(bi_dead);
        if (!dead_.valid())
        {
            LOG_ERROR("[particles] dead alloc failed");
            return false;
        }

        //~ the alive ping pong two index lists default heap uav
        for (std::uint32_t i = 0u; i < 2u; ++i)
        {
            hardware::buffer_create_info bi_alive{};
            bi_alive.size_bytes = k_alive_size_bytes;
            bi_alive.kind       = hardware::buffer_kind::default_uav;
            bi_alive.stride     = sizeof(std::uint32_t);
            bi_alive.debug_name = i == 0u ? "particle_alive_a" : "particle_alive_b";
            alive_[i] = bm.create(bi_alive);
            if (!alive_[i].valid())
            {
                LOG_ERROR("[particles] alive[{}] alloc failed", i);
                return false;
            }
        }

        //~ the counter buffer dead and alive counts default heap uav
        hardware::buffer_create_info bi_counter{};
        bi_counter.size_bytes = k_counter_size_bytes;
        bi_counter.kind       = hardware::buffer_kind::default_uav;
        bi_counter.stride     = sizeof(counter_layout);
        bi_counter.debug_name = "particle_counter";
        counter_ = bm.create(bi_counter);
        if (!counter_.valid())
        {
            LOG_ERROR("[particles] counter alloc failed");
            return false;
        }

        //~ the indirect dispatch args default heap uav
        hardware::buffer_create_info bi_sim{};
        bi_sim.size_bytes = k_sim_args_size_bytes;
        bi_sim.kind       = hardware::buffer_kind::default_uav;
        bi_sim.stride     = sizeof(dispatch_args_layout);
        bi_sim.debug_name = "particle_sim_args";
        sim_args_ = bm.create(bi_sim);
        if (!sim_args_.valid())
        {
            LOG_ERROR("[particles] sim_args alloc failed");
            return false;
        }

        //~ the indirect draw args default heap uav
        hardware::buffer_create_info bi_draw{};
        bi_draw.size_bytes = k_draw_args_size_bytes;
        bi_draw.kind       = hardware::buffer_kind::default_uav;
        bi_draw.stride     = sizeof(draw_args_layout);
        bi_draw.debug_name = "particle_draw_args";
        draw_args_ = bm.create(bi_draw);
        if (!draw_args_.valid())
        {
            LOG_ERROR("[particles] draw_args alloc failed");
            return false;
        }

        //~ the emitter ring one upload slot per frame in flight persistently
        //~ mapped so the cpu can write the table each frame without a fence
        for (std::uint32_t f = 0u; f < config::FRAME_COUNT; ++f)
        {
            hardware::buffer_create_info bi_em{};
            bi_em.size_bytes = k_emitter_ring_size_bytes;
            bi_em.kind       = hardware::buffer_kind::constant; //~ upload heap persistent map
            bi_em.stride     = sizeof(emitter_gpu);
            bi_em.debug_name = "particle_emitter_ring";
            emitter_ring_[f] = bm.create(bi_em);
            if (!emitter_ring_[f].valid())
            {
                LOG_ERROR("[particles] emitter_ring[{}] alloc failed", f);
                return false;
            }
        }

        return true;
    }

    bool particle_system::build_psos()
    {
        if (!device_ || !shaders_) return false;

        //~ init compute fills the dead list 0..MAX it is the one shot at startup
        //~ so it is not optional a missing init shader is a hard failure
        if (!build_compute_pso(*device_, *shaders_,
                               "assets/shaders/particle_init.hlsl", "CSMain",
                               init_rs_, init_pso_, L"particle init PSO",
                               false))
        {
            LOG_ERROR("[particles] init pso build failed");
            return false;
        }

        //~ the runtime kernels the optional flag lets the phase one build live
        //~ with just init seed and render while later phases drop the rest these
        //~ light up on their own once the files exist
        (void)build_compute_pso(*device_, *shaders_,
                                "assets/shaders/particle_reset_counts.hlsl", "CSMain",
                                reset_rs_, reset_pso_, L"particle reset counts PSO",
                                true);
        (void)build_compute_pso(*device_, *shaders_,
                                "assets/shaders/particle_build_sim_args.hlsl", "CSMain",
                                sim_args_rs_, sim_args_pso_, L"particle build sim args PSO",
                                true);
        (void)build_compute_pso(*device_, *shaders_,
                                "assets/shaders/particle_build_draw_args.hlsl", "CSMain",
                                draw_args_rs_, draw_args_pso_, L"particle build draw args PSO",
                                true);
        (void)build_compute_pso(*device_, *shaders_,
                                "assets/shaders/particle_simulate.hlsl", "CSMain",
                                simulate_rs_, simulate_pso_, L"particle simulate PSO",
                                true);
        (void)build_compute_pso(*device_, *shaders_,
                                "assets/shaders/particle_emit.hlsl", "CSMain",
                                emit_rs_, emit_pso_, L"particle emit PSO",
                                true);
        return true;
    }

    bool particle_system::build_command_signatures()
    {
        if (!device_) return false;
        auto* d3d = device_->d3d12_device();
        if (!d3d) return false;

        //~ the dispatch indirect signature one D3D12_DISPATCH_ARGUMENTS payload
        //~ the simulate path reads through ExecuteIndirect
        D3D12_INDIRECT_ARGUMENT_DESC dispatch_arg{};
        dispatch_arg.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

        D3D12_COMMAND_SIGNATURE_DESC dispatch_desc{};
        dispatch_desc.ByteStride       = sizeof(dispatch_args_layout);
        dispatch_desc.NumArgumentDescs = 1u;
        dispatch_desc.pArgumentDescs   = &dispatch_arg;
        dispatch_desc.NodeMask         = 0u;

        Microsoft::WRL::ComPtr<ID3D12CommandSignature> new_dispatch_sig;
        if (FAILED(d3d->CreateCommandSignature(&dispatch_desc, nullptr,
                                               IID_PPV_ARGS(&new_dispatch_sig))))
        {
            LOG_ERROR("[particles] CreateCommandSignature(DISPATCH) failed");
            return false;
        }
        new_dispatch_sig->SetName(L"particle dispatch sig");

        //~ the draw indirect signature one D3D12_DRAW_ARGUMENTS payload a plain
        //~ draw not indexed since the billboard expand carries no index buffer
        D3D12_INDIRECT_ARGUMENT_DESC draw_arg{};
        draw_arg.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

        D3D12_COMMAND_SIGNATURE_DESC draw_desc{};
        draw_desc.ByteStride       = sizeof(draw_args_layout);
        draw_desc.NumArgumentDescs = 1u;
        draw_desc.pArgumentDescs   = &draw_arg;
        draw_desc.NodeMask         = 0u;

        Microsoft::WRL::ComPtr<ID3D12CommandSignature> new_draw_sig;
        if (FAILED(d3d->CreateCommandSignature(&draw_desc, nullptr,
                                               IID_PPV_ARGS(&new_draw_sig))))
        {
            LOG_ERROR("[particles] CreateCommandSignature(DRAW) failed");
            return false;
        }
        new_draw_sig->SetName(L"particle draw sig");

        dispatch_sig_ = std::move(new_dispatch_sig);
        draw_sig_     = std::move(new_draw_sig);
        return true;
    }

    bool particle_system::run_init_compute(ID3D12CommandQueue* graphics_queue)
    {
        if (!device_ || !buffers_ || !init_pso_ || !init_rs_) return false;

        const std::uint64_t pool_addr    = buffers_->gpu_address(pool_);
        const std::uint64_t dead_addr    = buffers_->gpu_address(dead_);
        const std::uint64_t counter_addr = buffers_->gpu_address(counter_);
        if (!pool_addr || !dead_addr || !counter_addr) return false;

        return dispatch_init_kernel(graphics_queue,
                                    init_rs_.Get(), init_pso_.Get(),
                                    pool_addr, dead_addr, counter_addr);
    }

    bool particle_system::dispatch_init_kernel(
        ID3D12CommandQueue*  graphics_queue,
        ID3D12RootSignature* rs,
        ID3D12PipelineState* pso,
        const std::uint64_t  pool_addr,
        const std::uint64_t  dead_addr,
        const std::uint64_t  counter_addr)
    {
        auto* d3d = device_->d3d12_device();
        if (!d3d) return false;

        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> alloc;
        if (FAILED(d3d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                               IID_PPV_ARGS(&alloc))))
        {
            LOG_ERROR("[particles] init alloc create failed");
            return false;
        }
        alloc->SetName(L"particle init alloc");

        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> list;
        if (FAILED(d3d->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                           D3D12_COMMAND_LIST_FLAG_NONE,
                                           IID_PPV_ARGS(&list))))
        {
            LOG_ERROR("[particles] init list create failed");
            return false;
        }
        list->SetName(L"particle init list");

        if (FAILED(list->Reset(alloc.Get(), pso)))
        {
            LOG_ERROR("[particles] init list reset failed");
            return false;
        }

        list->SetComputeRootSignature(rs);

        const init_cb_t cb{ k_max_particles };
        list->SetComputeRoot32BitConstants(k_rp_init_cb, 1u, &cb, 0u);
        list->SetComputeRootUnorderedAccessView(k_rp_init_pool,    pool_addr);
        list->SetComputeRootUnorderedAccessView(k_rp_init_dead,    dead_addr);
        list->SetComputeRootUnorderedAccessView(k_rp_init_counter, counter_addr);

        const std::uint32_t groups = (k_max_particles + 63u) / 64u;
        list->Dispatch(groups, 1u, 1u);

        if (FAILED(list->Close()))
        {
            LOG_ERROR("[particles] init list close failed");
            return false;
        }

        ID3D12CommandList* lists[1] = { list.Get() };
        graphics_queue->ExecuteCommandLists(1u, lists);

        //~ waiting on the cpu through a throwaway fence the init only runs once
        Microsoft::WRL::ComPtr<ID3D12Fence1> fence;
        if (FAILED(d3d->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))))
        {
            LOG_ERROR("[particles] init fence create failed");
            return false;
        }
        fence->SetName(L"particle init fence");
        if (FAILED(graphics_queue->Signal(fence.Get(), 1u)))
        {
            LOG_ERROR("[particles] init signal failed");
            return false;
        }
        if (fence->GetCompletedValue() < 1u)
        {
            HANDLE evt = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
            if (!evt)
            {
                LOG_ERROR("[particles] init event create failed");
                return false;
            }
            if (FAILED(fence->SetEventOnCompletion(1u, evt)))
            {
                CloseHandle(evt);
                LOG_ERROR("[particles] init SetEventOnCompletion failed");
                return false;
            }
            WaitForSingleObject(evt, INFINITE);
            CloseHandle(evt);
        }
        return true;
    }

    void particle_system::rebuild_psos(hardware::deferred_releaser* releaser)
    {
        //~ rebuilding only the runtime kernels the init pso was one shot and
        //~ already dropped after the init dispatch
        if (!device_ || !shaders_) return;

        //~ deferring the live psos so the gpu drains its in flight references a
        //~ null releaser drops inline only safe right after a gpu flush
        if (releaser)
        {
            if (simulate_pso_)   releaser->enqueue_com(std::move(simulate_pso_));
            if (simulate_rs_)    releaser->enqueue_com(std::move(simulate_rs_));
            if (emit_pso_)       releaser->enqueue_com(std::move(emit_pso_));
            if (emit_rs_)        releaser->enqueue_com(std::move(emit_rs_));
            if (reset_pso_)      releaser->enqueue_com(std::move(reset_pso_));
            if (reset_rs_)       releaser->enqueue_com(std::move(reset_rs_));
            if (sim_args_pso_)   releaser->enqueue_com(std::move(sim_args_pso_));
            if (sim_args_rs_)    releaser->enqueue_com(std::move(sim_args_rs_));
            if (draw_args_pso_)  releaser->enqueue_com(std::move(draw_args_pso_));
            if (draw_args_rs_)   releaser->enqueue_com(std::move(draw_args_rs_));
        }
        else
        {
            simulate_pso_.Reset(); simulate_rs_.Reset();
            emit_pso_    .Reset(); emit_rs_    .Reset();
            reset_pso_   .Reset(); reset_rs_   .Reset();
            sim_args_pso_.Reset(); sim_args_rs_.Reset();
            draw_args_pso_.Reset(); draw_args_rs_.Reset();
        }

        //~ building only the runtime kernels never the init pso which would
        //~ dangle through to deinitialize
        (void)build_compute_pso(*device_, *shaders_,
                                "assets/shaders/particle_reset_counts.hlsl", "CSMain",
                                reset_rs_, reset_pso_, L"particle reset counts PSO", true);
        (void)build_compute_pso(*device_, *shaders_,
                                "assets/shaders/particle_build_sim_args.hlsl", "CSMain",
                                sim_args_rs_, sim_args_pso_, L"particle build sim args PSO", true);
        (void)build_compute_pso(*device_, *shaders_,
                                "assets/shaders/particle_build_draw_args.hlsl", "CSMain",
                                draw_args_rs_, draw_args_pso_, L"particle build draw args PSO", true);
        (void)build_compute_pso(*device_, *shaders_,
                                "assets/shaders/particle_simulate.hlsl", "CSMain",
                                simulate_rs_, simulate_pso_, L"particle simulate PSO", true);
        (void)build_compute_pso(*device_, *shaders_,
                                "assets/shaders/particle_emit.hlsl", "CSMain",
                                emit_rs_, emit_pso_, L"particle emit PSO", true);
    }

    bool particle_system::phase1_seed(hardware::device&         dev,
                                      hardware::buffer_manager& bm,
                                      ID3D12CommandQueue*       graphics_queue)
    {
        if (!ready_) return false;
        if (!device_ || !buffers_ || !shaders_) return false;
        if (!graphics_queue) return false;

        const auto cs = shaders_->get_or_compile(
            "assets/shaders/particle_phase1_seed.hlsl",
            "CSMain", shaders::shader_stage::compute);
        if (!cs.valid())
        {
            LOG_ERROR("[particles] phase1 seed shader compile failed");
            return false;
        }

        const shaders::shader_bytecode stages[] = { cs };
        std::vector<std::uint8_t> rs_blob;
        if (!shaders::build_program_root_sig(stages, rs_blob) || rs_blob.empty())
        {
            LOG_ERROR("[particles] phase1 seed root sig build failed");
            return false;
        }

        auto* d3d = device_->d3d12_device();
        if (!d3d) return false;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> rs;
        if (FAILED(d3d->CreateRootSignature(0, rs_blob.data(), rs_blob.size(),
                                            IID_PPV_ARGS(&rs))))
        {
            LOG_ERROR("[particles] phase1 seed CreateRootSignature failed");
            return false;
        }

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};
        desc.pRootSignature = rs.Get();
        desc.CS             = { cs.data, cs.size };
        Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
        if (FAILED(d3d->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pso))))
        {
            LOG_ERROR("[particles] phase1 seed pso create failed");
            return false;
        }

        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> alloc;
        if (FAILED(d3d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                               IID_PPV_ARGS(&alloc))))
            return false;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> list;
        if (FAILED(d3d->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                           D3D12_COMMAND_LIST_FLAG_NONE,
                                           IID_PPV_ARGS(&list))))
            return false;
        if (FAILED(list->Reset(alloc.Get(), pso.Get()))) return false;

        constexpr std::uint32_t k_seed_count = 1024u;
        struct seed_cb_t
        {
            std::uint32_t count;
            std::uint32_t pad0;
            std::uint32_t pad1;
            std::uint32_t pad2;
        } cb{};
        cb.count = k_seed_count;

        list->SetComputeRootSignature(rs.Get());
        //~ b0 four root constants then the uavs u0 pool u1 dead u2 alive_current
        //~ u3 counter u4 draw_args
        list->SetComputeRoot32BitConstants(0u, 4u, &cb, 0u);
        list->SetComputeRootUnorderedAccessView(1u, bm.gpu_address(pool_));
        list->SetComputeRootUnorderedAccessView(2u, bm.gpu_address(dead_));
        list->SetComputeRootUnorderedAccessView(3u, bm.gpu_address(alive_[alive_idx_]));
        list->SetComputeRootUnorderedAccessView(4u, bm.gpu_address(counter_));
        list->SetComputeRootUnorderedAccessView(5u, bm.gpu_address(draw_args_));

        const std::uint32_t groups = (k_seed_count + 63u) / 64u;
        list->Dispatch(groups, 1u, 1u);

        if (FAILED(list->Close())) return false;

        ID3D12CommandList* lists[1] = { list.Get() };
        graphics_queue->ExecuteCommandLists(1u, lists);

        Microsoft::WRL::ComPtr<ID3D12Fence1> fence;
        if (FAILED(d3d->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))))
            return false;
        if (FAILED(graphics_queue->Signal(fence.Get(), 1u))) return false;
        if (fence->GetCompletedValue() < 1u)
        {
            HANDLE evt = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
            if (!evt) return false;
            if (FAILED(fence->SetEventOnCompletion(1u, evt)))
            {
                CloseHandle(evt);
                return false;
            }
            WaitForSingleObject(evt, INFINITE);
            CloseHandle(evt);
        }

        (void)dev;
        LOG_INFO("[particles] phase1 seed wrote {} static particles", k_seed_count);
        return true;
    }

    void particle_system::swap_alive_lists() noexcept
    {
        alive_idx_ ^= 1u;
    }

    void particle_system::record_graphics_signal(const std::uint32_t current_frame,
                                                 const std::uint64_t value) noexcept
    {
        if (current_frame >= render_signals_.size()) return;
        render_signals_[current_frame] = value;
    }

    std::uint64_t particle_system::prior_frame_graphics_signal(
        const std::uint32_t current_frame) const noexcept
    {
        constexpr std::uint32_t fc = config::FRAME_COUNT;
        const std::uint32_t prior = (current_frame + fc - 1u) % fc;
        if (prior >= render_signals_.size()) return 0u;
        return render_signals_[prior];
    }

    std::uint64_t particle_system::consume_alive_address() const noexcept
    {
        if (!buffers_) return 0u;
        return buffers_->gpu_address(alive_[alive_idx_]);
    }

    ID3D12Resource* particle_system::consume_alive_resource() const noexcept
    {
        if (!buffers_) return nullptr;
        return buffers_->resource(alive_[alive_idx_]);
    }

    ID3D12Resource* particle_system::pool_resource() const
    {
        return buffers_ ? buffers_->resource(pool_) : nullptr;
    }
    ID3D12Resource* particle_system::dead_resource() const
    {
        return buffers_ ? buffers_->resource(dead_) : nullptr;
    }
    ID3D12Resource* particle_system::alive_current_resource() const
    {
        return buffers_ ? buffers_->resource(alive_[alive_idx_]) : nullptr;
    }
    ID3D12Resource* particle_system::alive_next_resource() const
    {
        return buffers_ ? buffers_->resource(alive_[alive_idx_ ^ 1u]) : nullptr;
    }
    ID3D12Resource* particle_system::counter_resource() const
    {
        return buffers_ ? buffers_->resource(counter_) : nullptr;
    }
    ID3D12Resource* particle_system::sim_args_resource() const
    {
        return buffers_ ? buffers_->resource(sim_args_) : nullptr;
    }
    ID3D12Resource* particle_system::draw_args_resource() const
    {
        return buffers_ ? buffers_->resource(draw_args_) : nullptr;
    }
    ID3D12Resource* particle_system::emitter_resource(const std::uint32_t frame) const
    {
        if (!buffers_ || frame >= emitter_ring_.size()) return nullptr;
        return buffers_->resource(emitter_ring_[frame]);
    }

    std::uint64_t particle_system::pool_address() const
    {
        return buffers_ ? buffers_->gpu_address(pool_) : 0u;
    }
    std::uint64_t particle_system::dead_address() const
    {
        return buffers_ ? buffers_->gpu_address(dead_) : 0u;
    }
    std::uint64_t particle_system::alive_current_address() const
    {
        return buffers_ ? buffers_->gpu_address(alive_[alive_idx_]) : 0u;
    }
    std::uint64_t particle_system::alive_next_address() const
    {
        return buffers_ ? buffers_->gpu_address(alive_[alive_idx_ ^ 1u]) : 0u;
    }
    std::uint64_t particle_system::counter_address() const
    {
        return buffers_ ? buffers_->gpu_address(counter_) : 0u;
    }
    std::uint64_t particle_system::sim_args_address() const
    {
        return buffers_ ? buffers_->gpu_address(sim_args_) : 0u;
    }
    std::uint64_t particle_system::draw_args_address() const
    {
        return buffers_ ? buffers_->gpu_address(draw_args_) : 0u;
    }
    std::uint64_t particle_system::emitter_address(const std::uint32_t frame) const
    {
        if (!buffers_ || frame >= emitter_ring_.size()) return 0u;
        return buffers_->gpu_address(emitter_ring_[frame]);
    }

    std::uint32_t particle_system::pending_spawn_count(
        const std::uint32_t frame) const noexcept
    {
        if (frame >= emit_state_.size()) return 0u;
        return emit_state_[frame].spawn_count;
    }

    std::uint32_t particle_system::pending_emitter_count(
        const std::uint32_t frame) const noexcept
    {
        if (frame >= emit_state_.size()) return 0u;
        return emit_state_[frame].emitter_count;
    }

    std::uint32_t particle_system::begin_frame_build_emitters(
        const std::uint32_t   frame,
        const float           delta_time,
        const scene_snapshot* snap)
    {
        if (frame >= emit_state_.size())   return 0u;
        if (!buffers_ || !snap)
        {
            if (frame < emit_state_.size()) emit_state_[frame] = {};
            return 0u;
        }

        ++frame_counter_;

        emit_state_[frame] = {};
        const auto& src = snap->particle_emitters;
        if (src.empty()) return 0u;

        auto* dst = static_cast<emitter_gpu*>(
            buffers_->mapped_ptr(emitter_ring_[frame]));
        if (!dst) return 0u;

        std::uint32_t total_spawn   = 0u;
        std::uint32_t emitter_count = 0u;

        for (std::size_t i = 0; i < src.size() && emitter_count < k_max_emitters_per_frame; ++i)
        {
            const auto& es = src[i];
            //~ finding this emitters carry slot or appending a fresh one the
            //~ carry rides across frames so fractional spawns add up
            carry_entry* slot = nullptr;
            for (auto& e : carry_)
            {
                if (e.emitter_id == es.emitter_id)
                {
                    slot = &e;
                    break;
                }
            }
            if (!slot)
            {
                carry_.push_back({ es.emitter_id, 0.f, 0u });
                slot = &carry_.back();
            }
            slot->last_frame = frame_counter_;

            //~ carry plus rate times dt then floor so a low rate like 0.3 builds
            //~ up to one spawn every few frames instead of rounding to zero
            const float requested = slot->remainder + es.spawn_rate * delta_time;
            std::uint32_t spawn   = static_cast<std::uint32_t>(std::floor(requested));
            slot->remainder       = requested - static_cast<float>(spawn);

            if (total_spawn + spawn > k_max_spawns_per_frame)
            {
                spawn = (k_max_spawns_per_frame > total_spawn)
                    ? k_max_spawns_per_frame - total_spawn
                    : 0u;
            }
            if (spawn == 0u) continue;

            emitter_gpu rec{};
            rec.type             = static_cast<std::uint32_t>(es.type);
            rec.position[0]      = es.position[0];
            rec.position[1]      = es.position[1];
            rec.position[2]      = es.position[2];
            rec.spawn_radius     = es.radius;
            rec.min_lifetime     = es.min_lifetime;
            rec.max_lifetime     = es.max_lifetime;
            rec.start_size       = es.start_size;
            rec.base_color       = es.base_color;
            rec.spawn_count      = spawn;
            rec.first_thread     = total_spawn;
            rec.pad0             = 0u;

            std::memcpy(dst + emitter_count, &rec, sizeof(rec));
            ++emitter_count;
            total_spawn += spawn;
            if (total_spawn >= k_max_spawns_per_frame) break;
        }

        //~ pruning carry slots that have gone quiet for a while keeps the vector
        //~ bounded over a long session an emitter that drops out for sixty
        //~ frames falls out and resets its carry next time it shows up
        carry_.erase(
            std::remove_if(carry_.begin(), carry_.end(),
                [this](const carry_entry& e)
                {
                    return e.last_frame + 60u < frame_counter_;
                }),
            carry_.end());

        emit_state_[frame].spawn_count   = total_spawn;
        emit_state_[frame].emitter_count = emitter_count;
        return total_spawn;
    }
} // namespace trishul::render::particles
