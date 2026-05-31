#define PARTICLE_INIT_RS \
    "RootConstants(num32BitConstants=1, b0)," \
    "UAV(u0)," \
    "UAV(u1)," \
    "UAV(u2)"

cbuffer InitCB : register(b0)
{
    uint g_max_particles;
};

#include "particle_common.hlsli"

RWStructuredBuffer<Particle>        g_pool    : register(u0);
RWStructuredBuffer<uint>            g_dead    : register(u1);
RWStructuredBuffer<ParticleCounter> g_counter : register(u2);

[RootSignature(PARTICLE_INIT_RS)]
[numthreads(64, 1, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    const uint i = dtid.x;
    if (i >= g_max_particles) return;

    //~ dead[i] = i the whole pool starts dead an emit pops the tail and writes
    //~ that index into the pool and the alive list a death pushes it back here
    g_dead[i] = i;

    //~ zeroing the pool entry so a debugger view of an unused slot reads clean
    //~ and a stray draw of a dead slot lands at zero alpha
    Particle p;
    p.position     = float3(0.0, 0.0, 0.0);
    p.velocity     = float3(0.0, 0.0, 0.0);
    p.life         = 0.0;
    p.max_life     = 0.0;
    p.size         = 0.0;
    p.color_seed   = 0;
    p.emitter_type = 0u;
    p.pad1         = 0;
    g_pool[i]      = p;

    //~ thread zero seeds the counter block dead=MAX the rest zero the emit
    //~ clamps against dead_count so the pop path never underflows
    if (i == 0)
    {
        g_counter[0].dead_count          = g_max_particles;
        g_counter[0].alive_count_current = 0;
        g_counter[0].alive_count_next    = 0;
        g_counter[0].pad0                = 0;
    }
}
