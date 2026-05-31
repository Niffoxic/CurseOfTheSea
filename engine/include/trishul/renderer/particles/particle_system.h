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
#ifndef CURSEOFTHESEA_PARTICLE_SYSTEM_H
#define CURSEOFTHESEA_PARTICLE_SYSTEM_H

#include <array>
#include <cstdint>
#include <vector>
#include <wrl/client.h>

#include "trishul/renderer/particles/particle_data.h"
#include "trishul/renderer/hardware/resource.h"
#include "trishul/core/engine_config.h"

struct ID3D12CommandSignature;
struct ID3D12PipelineState;
struct ID3D12RootSignature;
struct ID3D12Resource;
struct ID3D12CommandQueue;

namespace trishul::render
{
    namespace hardware
    {
        class device;
        class buffer_manager;
        class deferred_releaser;
    }
    namespace shaders { class shader_cache; }
    struct scene_snapshot;
}

namespace trishul::render::particles
{
    //~ engine wide cap on emitters alive in one frame the emit shader binds an
    //~ srv this size so the cpu drops anything over budget quietly
    constexpr std::uint32_t k_max_emitters_per_frame = 32u;

    //~ engine wide cap on spawns this frame across every emitter the emit
    //~ dispatch sizes against this to keep the per frame cost bounded the cpu
    //~ clamps each emitters request so the sum stays under the cap
    constexpr std::uint32_t k_max_spawns_per_frame = 4096u;

    //~ owns every gpu buffer the particle pipeline touches the pool the dead
    //~ list the alive ping pong the counters the indirect args the emitter
    //~ upload ring plus the command signatures for the indirect dispatch and
    //~ draw the renderer keeps one of these long lived so the persistent buffers
    //~ ride every frame the alive lists and counters never live on the cpu the
    //~ cpu only flips which buffer is current vs next
    class particle_system final
    {
    public:
         particle_system() = default;
        ~particle_system() = default;

        particle_system           (const particle_system&) = delete;
        particle_system& operator=(const particle_system&) = delete;
        particle_system           (particle_system&&)      = delete;
        particle_system& operator=(particle_system&&)      = delete;

        //~ allocating every persistent buffer then running the one shot init
        //~ compute that seeds the dead list 0..MAX with dead_count = MAX it runs
        //~ synchronously on the graphics queue so everything is ready before the
        //~ first record_frame
        [[nodiscard]] bool initialize(hardware::device&         dev,
                                      hardware::buffer_manager& bm,
                                      shaders::shader_cache&    shaders,
                                      ID3D12CommandQueue*       graphics_queue);

        //~ releasing every buffer and com pointer
        void deinitialize() noexcept;

        //~ rebuilding every runtime compute pso the shader hot reload path calls
        //~ this after a shader_cache recompile it defers the old psos through the
        //~ releaser before building the fresh ones
        void rebuild_psos(hardware::deferred_releaser* releaser);

        //~ flipping alive_current and alive_next after a frames simulate next
        //~ frame reads the freshly swapped current and writes the empty next the
        //~ swap is cpu state only the gpu buffers stay put
        void swap_alive_lists() noexcept;

        //~ pso plus root sig accessors
        [[nodiscard]] ID3D12PipelineState* simulate_pso () const noexcept { return simulate_pso_.Get(); }
        [[nodiscard]] ID3D12RootSignature* simulate_rs  () const noexcept { return simulate_rs_.Get(); }
        [[nodiscard]] ID3D12PipelineState* emit_pso     () const noexcept { return emit_pso_.Get(); }
        [[nodiscard]] ID3D12RootSignature* emit_rs      () const noexcept { return emit_rs_.Get(); }
        [[nodiscard]] ID3D12PipelineState* reset_pso    () const noexcept { return reset_pso_.Get(); }
        [[nodiscard]] ID3D12RootSignature* reset_rs     () const noexcept { return reset_rs_.Get(); }
        [[nodiscard]] ID3D12PipelineState* sim_args_pso () const noexcept { return sim_args_pso_.Get(); }
        [[nodiscard]] ID3D12RootSignature* sim_args_rs  () const noexcept { return sim_args_rs_.Get(); }
        [[nodiscard]] ID3D12PipelineState* draw_args_pso() const noexcept { return draw_args_pso_.Get(); }
        [[nodiscard]] ID3D12RootSignature* draw_args_rs () const noexcept { return draw_args_rs_.Get(); }

        [[nodiscard]] ID3D12CommandSignature* dispatch_cmd_sig() const noexcept { return dispatch_sig_.Get(); }
        [[nodiscard]] ID3D12CommandSignature* draw_cmd_sig    () const noexcept { return draw_sig_.Get();   }

        [[nodiscard]] ID3D12Resource* pool_resource()           const;
        [[nodiscard]] ID3D12Resource* dead_resource()           const;
        [[nodiscard]] ID3D12Resource* alive_current_resource()  const;
        [[nodiscard]] ID3D12Resource* alive_next_resource()     const;
        [[nodiscard]] ID3D12Resource* counter_resource()        const;
        [[nodiscard]] ID3D12Resource* sim_args_resource()       const;
        [[nodiscard]] ID3D12Resource* draw_args_resource()      const;
        [[nodiscard]] ID3D12Resource* emitter_resource(std::uint32_t frame) const;

        [[nodiscard]] std::uint64_t pool_address()           const;
        [[nodiscard]] std::uint64_t dead_address()           const;
        [[nodiscard]] std::uint64_t alive_current_address()  const;
        [[nodiscard]] std::uint64_t alive_next_address()     const;
        [[nodiscard]] std::uint64_t counter_address()        const;
        [[nodiscard]] std::uint64_t sim_args_address()       const;
        [[nodiscard]] std::uint64_t draw_args_address()      const;
        [[nodiscard]] std::uint64_t emitter_address(std::uint32_t frame) const;

        //~ phase one seeding writes a fixed block of particles into alive_current
        //~ and seeds the counters so the render pass has something to draw goes
        //~ away in phase three once the emit shader takes over generation
        bool phase1_seed(hardware::device&         dev,
                         hardware::buffer_manager& bm,
                         ID3D12CommandQueue*       graphics_queue);

        //~ the cross frame fence the renderer records the terminal graphics
        //~ signal of every frame and the update pass posts a gpu wait on the
        //~ prior frames signal so compute(N+1) does not clobber the pool while
        //~ graphics(N) is still reading it slot rotation matches the frames in
        //~ flight setup
        void record_graphics_signal(std::uint32_t current_frame,
                                    std::uint64_t value) noexcept;
        [[nodiscard]] std::uint64_t prior_frame_graphics_signal(
            std::uint32_t current_frame) const noexcept;

        //~ the buffer the render pass consumes survivors of this frames simulate
        //~ land in alive_next which is why the cpu side swap happens after the
        //~ frame closes
        [[nodiscard]] std::uint64_t consume_alive_address() const noexcept;
        [[nodiscard]] ID3D12Resource* consume_alive_resource() const noexcept;

        //~ true once initialize and the init compute pass both ran a false here
        //~ short circuits both the update and render passes
        [[nodiscard]] bool is_ready() const noexcept { return ready_; }

        //~ host side spawn budgeting the renderer calls this at the start of
        //~ every record_frame to size the emit dispatch and upload the emitter
        //~ table the per emitter fractional carry lives in here returns the
        //~ total spawn count this frame the update pass dispatches that many
        std::uint32_t begin_frame_build_emitters(
            std::uint32_t      frame,
            float              delta_time,
            const scene_snapshot* snap);

        [[nodiscard]] std::uint32_t pending_spawn_count(std::uint32_t frame) const noexcept;
        [[nodiscard]] std::uint32_t pending_emitter_count(std::uint32_t frame) const noexcept;

        //~ frame counter the gpu side rng seeds hash on this
        [[nodiscard]] std::uint32_t frame_counter() const noexcept { return frame_counter_; }

    private:
        bool build_psos();
        bool build_command_signatures();
        bool allocate_buffers(hardware::buffer_manager& bm);
        bool run_init_compute(ID3D12CommandQueue* graphics_queue);

        //~ running a one shot compute kernel synchronously the init path records
        //~ a tiny direct list on the graphics queue executes signals a temp
        //~ fence waits then drops the temp objects it only runs once at startup
        //~ so the per call alloc is fine
        bool dispatch_init_kernel(ID3D12CommandQueue* graphics_queue,
                                  ID3D12RootSignature* rs,
                                  ID3D12PipelineState* pso,
                                  std::uint64_t        pool_addr,
                                  std::uint64_t        dead_addr,
                                  std::uint64_t        counter_addr);

        hardware::device*         device_  { nullptr };
        hardware::buffer_manager* buffers_ { nullptr };
        shaders::shader_cache*    shaders_ { nullptr };

        //~ persistent buffers default heap uav capable
        hardware::buffer_handle pool_           {};
        hardware::buffer_handle dead_           {};
        std::array<hardware::buffer_handle, 2u> alive_ {};
        hardware::buffer_handle counter_        {};
        hardware::buffer_handle sim_args_       {};
        hardware::buffer_handle draw_args_      {};

        //~ per frame emitter upload buffer one ring slot per frame in flight
        std::array<hardware::buffer_handle, config::FRAME_COUNT> emitter_ring_{};

        //~ alive ping pong cpu side index
        std::uint32_t alive_idx_ { 0u };

        //~ the cross frame fence values one per frame slot
        std::array<std::uint64_t, config::FRAME_COUNT> render_signals_{};

        //~ per frame emitter cache
        struct frame_emit_state
        {
            std::uint32_t spawn_count   { 0u };
            std::uint32_t emitter_count { 0u };
        };
        std::array<frame_emit_state, config::FRAME_COUNT> emit_state_{};

        //~ per emitter fractional spawn carry keyed by the emitter id the
        //~ gameplay supplies so low rates do not round to zero every frame the
        //~ last_frame stamp expires the carry after a few idle frames keeping
        //~ the vector bounded
        struct carry_entry
        {
            std::uint32_t emitter_id      { 0u };
            float         remainder       { 0.f };
            std::uint64_t last_frame      { 0u };
        };
        std::vector<carry_entry> carry_{};

        std::uint32_t frame_counter_ { 0u };

        Microsoft::WRL::ComPtr<ID3D12RootSignature> simulate_rs_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> simulate_pso_;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> emit_rs_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> emit_pso_;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> reset_rs_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> reset_pso_;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> sim_args_rs_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> sim_args_pso_;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> draw_args_rs_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> draw_args_pso_;

        //~ init compute is one shot built at startup torn down right after the
        //~ init dispatch returns
        Microsoft::WRL::ComPtr<ID3D12RootSignature> init_rs_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> init_pso_;

        Microsoft::WRL::ComPtr<ID3D12CommandSignature> dispatch_sig_;
        Microsoft::WRL::ComPtr<ID3D12CommandSignature> draw_sig_;

        bool ready_ { false };
    };
} // namespace trishul::render::particles

#endif //CURSEOFTHESEA_PARTICLE_SYSTEM_H
