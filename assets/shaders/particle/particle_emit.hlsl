#define PARTICLE_EMIT_RS \
    "RootConstants(num32BitConstants=4, b0)," \
    "SRV(t0)," \
    "UAV(u0)," \
    "UAV(u1)," \
    "UAV(u2)," \
    "UAV(u3)"

cbuffer EmitCB : register(b0)
{
    uint g_total_spawn;
    uint g_emitter_count;
    uint g_frame_counter;
    uint g_pad0;
};

#include "particle_common.hlsli"

StructuredBuffer<ParticleEmitter>   g_emitters       : register(t0);
RWStructuredBuffer<Particle>        g_pool           : register(u0);
RWStructuredBuffer<uint>            g_dead           : register(u1);
RWStructuredBuffer<uint>            g_alive_current  : register(u2);
RWStructuredBuffer<ParticleCounter> g_counter        : register(u3);

//~ finding which emitter owns this thread by walking the table it is tiny under
//~ the engine cap so a linear scan beats any tree first_thread is the prefix sum
//~ the cpp side built in begin_frame_build_emitters
uint find_emitter_index(uint thread_id, out uint local_idx)
{
    local_idx = 0;
    for (uint i = 0; i < g_emitter_count; ++i)
    {
        const uint start = g_emitters[i].first_thread;
        const uint end   = start + g_emitters[i].spawn_count;
        if (thread_id >= start && thread_id < end)
        {
            local_idx = thread_id - start;
            return i;
        }
    }
    return 0xffffffffu;
}

//~ popping a dead slot guarded against underflow returns the invalid sentinel
//~ if the list is empty the cpp budgeting clamps so this only fires on the rare
//~ race where simulates dead push has not landed yet
uint pop_dead_slot()
{
    uint slot_idx;
    InterlockedAdd(g_counter[0].dead_count, -1, slot_idx);
    if (slot_idx == 0)
    {
        //~ we just went under zero on an unsigned counter put it back so the
        //~ next thread does not read a huge wrapped value simulate refills it
        InterlockedAdd(g_counter[0].dead_count, 1u);
        return 0xffffffffu;
    }
    return g_dead[slot_idx - 1];
}

[RootSignature(PARTICLE_EMIT_RS)]
[numthreads(64, 1, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    const uint thread_id = dtid.x;
    if (thread_id >= g_total_spawn) return;

    uint local_idx;
    const uint em_idx = find_emitter_index(thread_id, local_idx);
    if (em_idx == 0xffffffffu) return;

    const uint pool_idx = pop_dead_slot();
    if (pool_idx == 0xffffffffu) return;

    const ParticleEmitter em = g_emitters[em_idx];

    //~ the rng seed folds frame thread and emitter together so two emitters
    //~ never share a sequence even when their thread ranges sit next to each other
    const uint seed = particle_hash_u32(
        thread_id ^ (em_idx * 0x9e3779b9u) ^ (g_frame_counter * 0x85ebca6bu));

    //~ spawn position inside a sphere of em.spawn_radius cube the radius pull to
    //~ bias toward the centre it is not a uniform sphere but fire clusters at
    //~ the middle anyway so it reads right
    const float r_factor = pow(particle_rand01(seed ^ 0x1u), 1.0 / 3.0);
    const float theta    = particle_rand01(seed ^ 0x2u) * 6.28318530718;
    const float phi      = particle_rand01(seed ^ 0x3u) * 3.14159265359;
    const float r        = em.spawn_radius * r_factor;
    const float sx       = r * sin(phi) * cos(theta);
    const float sy       = r * cos(phi);
    const float sz       = r * sin(phi) * sin(theta);

    const float3 spawn_pos = float3(em.position) + float3(sx, sy * 0.3, sz);

    //~ per type spawn params fire pushes up hard with a tight cone smoke rises
    //~ slower drifts wider and starts bigger so a campfire feels like heat
    //~ shoving flame up and smoke plumes out over time
    const float3 outward = normalize(float3(sx, 0.01, sz) + float3(0.0001, 0.0, 0.0));
    float3 velocity;
    float  life;
    float  size;
    uint   color;

    if (em.type == 1u) //~ smoke
    {
        const float speed   = particle_rand_range(seed ^ 0x10u, 0.3, 0.8);
        const float up      = particle_rand_range(seed ^ 0x20u, 0.6, 1.2);
        velocity = float3(outward.x * speed,
                          up,
                          outward.z * speed);
        //~ smoke lives longer and starts chunkier clamped inside the emitter min
        //~ max so the alive budget stays predictable
        life  = particle_rand_range(seed ^ 0x30u,
                                    em.min_lifetime,
                                    em.max_lifetime);
        size  = em.start_size *
                particle_rand_range(seed ^ 0x40u, 1.4, 2.0);
        //~ warm grey start the pixel shader fades toward cooler grey alpha at
        //~ one keeps the per fragment fade math simple
        const float4 start_col = float4(particle_smoke_color(0.0), 1.0);
        color = particle_pack_unorm_color(saturate(start_col));
    }
    else if (em.type == 2u) //~ spray sea splash off a wave crest
    {
        //~ water flicks up and outward then arcs back down a strong vertical
        //~ kick a wide cone and a short life reads as droplets off a breaking
        //~ wave not a slow plume the emitter base_color carries the foamy tint
        const float speed = particle_rand_range(seed ^ 0x10u, 1.2, 3.2);
        const float up    = particle_rand_range(seed ^ 0x20u, 2.5, 5.5);
        velocity = float3(outward.x * speed,
                          up,
                          outward.z * speed);
        life = particle_rand_range(seed ^ 0x30u,
                                   em.min_lifetime,
                                   em.max_lifetime);
        size = em.start_size *
               particle_rand_range(seed ^ 0x40u, 0.4, 1.0);
        color = em.base_color;
    }
    else //~ fire the default
    {
        const float speed = particle_rand_range(seed ^ 0x10u, 1.5, 3.5);
        const float up    = particle_rand_range(seed ^ 0x20u, 2.0, 3.5);
        velocity = float3(outward.x * speed * 0.4,
                          up,
                          outward.z * speed * 0.4);
        life = particle_rand_range(seed ^ 0x30u,
                                   em.min_lifetime,
                                   em.max_lifetime);
        size = em.start_size *
               particle_rand_range(seed ^ 0x40u, 0.7, 1.2);
        const float4 start_col = float4(particle_fire_color(0.0), 1.0);
        color = particle_pack_unorm_color(saturate(start_col / 1.5));
    }

    Particle p;
    p.position     = spawn_pos;
    p.velocity     = velocity;
    p.life         = life;
    p.max_life     = life;
    p.size         = size;
    p.color_seed   = color;
    p.emitter_type = em.type;
    p.pad1         = 0;
    g_pool[pool_idx] = p;

    //~ appending to alive_current simulate reads from here this same frame once
    //~ build_sim_args has run against the post emit count so the new particles
    //~ get simulated this frame instead of waiting one
    uint alive_pos;
    InterlockedAdd(g_counter[0].alive_count_current, 1u, alive_pos);
    g_alive_current[alive_pos] = pool_idx;
}
