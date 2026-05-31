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
#ifndef CURSEOFTHESEA_PARTICLE_DATA_H
#define CURSEOFTHESEA_PARTICLE_DATA_H

#include <cstdint>
#include <DirectXMath.h>

namespace trishul::render::particles
{
    //~ the pool budget the dead and alive lists size around this spawn requests
    //~ clamp against the running dead count so going over the cap just drops the
    //~ extra spawns quietly pushing past 256k means a buffer alloc bump but the
    //~ rest of the data path does not care about the number
    constexpr std::uint32_t k_max_particles = 256u * 1024u;

    //~ one particle the aos record matches the HLSL Particle in
    //~ particle_common.hlsli the 48 byte stride leaves a pad pair so a future
    //~ curl noise variant can ride along without a buffer rebuild the
    //~ static_assert pins it against the gpu side stride
    struct particle
    {
        DirectX::XMFLOAT3 position     { 0.f, 0.f, 0.f }; //~ 12
        float             life         { 0.f };           //~  4  -> 16
        DirectX::XMFLOAT3 velocity     { 0.f, 0.f, 0.f }; //~ 12
        float             max_life     { 0.f };           //~  4  -> 32
        float             size         { 0.f };           //~  4
        std::uint32_t     color_seed   { 0u };            //~  4
        //~ emitter_type rides per particle so the render shader can branch the
        //~ look without an emitter lookup it matches particles::particle_type
        std::uint32_t     emitter_type { 0u };            //~  4
        std::uint32_t     pad1         { 0u };            //~  4  -> 48
    };
    static_assert(sizeof(particle) == 48u,
                  "particle stride must match the gpu side Particle struct");

    //~ the counter buffer three live uints the reset emit and build args kernels
    //~ InterlockedAdd against these dead_count tracks free pool slots
    //~ alive_count_current is last frames survivors plus this frames emits
    //~ alive_count_next is filled by simulate the cpu reads none of this the gpu
    //~ owns every field
    struct counter_layout
    {
        std::uint32_t dead_count         { 0u };
        std::uint32_t alive_count_current{ 0u };
        std::uint32_t alive_count_next   { 0u };
        std::uint32_t pad0               { 0u };
    };
    static_assert(sizeof(counter_layout) == 16u,
                  "counter_layout must stay 16 bytes for the gpu side counter buffer");

    //~ the indirect dispatch args one record driving the simulate ExecuteIndirect
    //~ build_sim_args writes the three group fields each frame from the alive
    //~ count ceil divided by the group size matches D3D12_DISPATCH_ARGUMENTS
    struct dispatch_args_layout
    {
        std::uint32_t thread_groups_x { 0u };
        std::uint32_t thread_groups_y { 0u };
        std::uint32_t thread_groups_z { 0u };
    };
    static_assert(sizeof(dispatch_args_layout) == 12u,
                  "dispatch_args_layout must match D3D12_DISPATCH_ARGUMENTS");

    //~ the indirect draw args one record driving the particle render
    //~ ExecuteIndirect build_draw_args sets instance_count from alive_count_next
    //~ each frame matches D3D12_DRAW_ARGUMENTS
    struct draw_args_layout
    {
        std::uint32_t vertex_count_per_instance { 6u };
        std::uint32_t instance_count            { 0u };
        std::uint32_t start_vertex_location     { 0u };
        std::uint32_t start_instance_location   { 0u };
    };
    static_assert(sizeof(draw_args_layout) == 16u,
                  "draw_args_layout must match D3D12_DRAW_ARGUMENTS");

    //~ the simulate threadgroup size matching numthreads in particle_simulate.hlsl
    //~ bumping one side means bumping the other so the build_sim_args ceil divide
    //~ stays right
    constexpr std::uint32_t k_simulate_threads_per_group = 64u;

    //~ the emit threadgroup size matching numthreads in particle_emit.hlsl one
    //~ thread per particle to spawn the cpu ceil divides the spawn count by this
    constexpr std::uint32_t k_emit_threads_per_group = 64u;

    //~ the engine known particle types the gameplay enum mirrored on the gpu
    //~ side the emit simulate and render shaders branch on these so keep the
    //~ numbers lined up with the hlsl
    enum class particle_type : std::uint32_t
    {
        fire  = 0u,
        smoke = 1u,
        spray = 2u,   //~ sea splash off a wave crest falls under real gravity
    };

    //~ one emitter record uploaded once per frame to a structured buffer the
    //~ emit shader reads one per thread group spawn_count plus first_thread
    //~ drive the per thread emitter lookup
    struct emitter_gpu
    {
        std::uint32_t      type             { 0u };
        float              position[3]      { 0.f, 0.f, 0.f };
        float              spawn_radius     { 0.f };
        float              min_lifetime     { 0.f };
        float              max_lifetime     { 0.f };
        float              start_size       { 0.f };
        std::uint32_t      base_color       { 0xffffffffu };
        std::uint32_t      spawn_count      { 0u };
        std::uint32_t      first_thread     { 0u };
        std::uint32_t      pad0             { 0u };
    };
    static_assert(sizeof(emitter_gpu) == 48u,
                  "emitter_gpu must match the gpu side Emitter struct");
} // namespace trishul::render::particles

#endif //CURSEOFTHESEA_PARTICLE_DATA_H
