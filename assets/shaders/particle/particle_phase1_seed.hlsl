#define PARTICLE_PHASE1_SEED_RS \
    "RootConstants(num32BitConstants=4, b0)," \
    "UAV(u0)," \
    "UAV(u1)," \
    "UAV(u2)," \
    "UAV(u3)," \
    "UAV(u4)"

cbuffer SeedCB : register(b0)
{
    uint g_count;
    uint g_pad0;
    uint g_pad1;
    uint g_pad2;
};

#include "particle_common.hlsli"

RWStructuredBuffer<Particle>        g_pool          : register(u0);
RWStructuredBuffer<uint>            g_dead          : register(u1);
RWStructuredBuffer<uint>            g_alive_current : register(u2);
RWStructuredBuffer<ParticleCounter> g_counter       : register(u3);

struct DrawArgs
{
    uint vertex_count_per_instance;
    uint instance_count;
    uint start_vertex_location;
    uint start_instance_location;
};
RWStructuredBuffer<DrawArgs> g_draw_args : register(u4);

[RootSignature(PARTICLE_PHASE1_SEED_RS)]
[numthreads(64, 1, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    const uint i = dtid.x;
    if (i >= g_count) return;

    //~ popping a slot off the dead list init filled it 0..MAX so the tail hands
    //~ back the high indices first this runs once at startup so dead_count just
    //~ drops from MAX to MAX minus g_count
    uint slot_idx;
    InterlockedAdd(g_counter[0].dead_count, -1, slot_idx);
    //~ slot_idx is the count before the decrement so the popped index sits at
    //~ slot_idx - 1
    const uint dead_pos = slot_idx - 1;
    const uint pool_idx = g_dead[dead_pos];

    //~ a static 32 by 32 grid from the linear thread id rows and columns spaced
    //~ out and centred so the cloud floats above the ground for any camera
    const uint cols = 32;
    const uint col  = i % cols;
    const uint row  = (i / cols) % cols;
    const float spacing = 0.40;
    const float ox = float(col) * spacing - (float(cols) * spacing * 0.5);
    const float oy = float(row) * spacing - (float(cols) * spacing * 0.5);

    Particle p;
    p.position   = float3(ox, oy + 6.0, 0.0);
    p.velocity   = float3(0.0, 0.0, 0.0);
    p.life       = 1.0;                 //~ stays alive forever in phase one
    p.max_life   = 1.0;
    p.size       = 0.18;
    //~ tinting by hash so the grid reads like a textured cloud not a flat slab
    //~ the radial falloff in the pixel shader softens the edges
    const float3 c = particle_fire_color(particle_rand01(i * 17u + 31u));
    p.color_seed   = particle_pack_unorm_color(float4(c, 0.7));
    p.emitter_type = 0u;   //~ seeded particles render as fire
    p.pad1         = 0;
    g_pool[pool_idx] = p;

    //~ appending to alive_current the cpu stays at alive_idx 0 so the render
    //~ pass reads alive_[0] we stamp both alive slots so the first reset is a no
    //~ op and simulate picks up the seed count straight away
    uint alive_pos;
    InterlockedAdd(g_counter[0].alive_count_current, 1u, alive_pos);
    g_alive_current[alive_pos] = pool_idx;
    InterlockedAdd(g_counter[0].alive_count_next, 1u);

    //~ thread zero stamps the draw args once for the whole seed the cpu reads
    //~ consume_alive_address off the same alive_idx so the indirect draw lines up
    if (i == 0)
    {
        g_draw_args[0].vertex_count_per_instance = 6;
        g_draw_args[0].instance_count            = g_count;
        g_draw_args[0].start_vertex_location     = 0;
        g_draw_args[0].start_instance_location   = 0;
    }
}
