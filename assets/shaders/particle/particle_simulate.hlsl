#define PARTICLE_SIMULATE_RS \
    "RootConstants(num32BitConstants=8, b0)," \
    "UAV(u0)," \
    "UAV(u1)," \
    "UAV(u2)," \
    "UAV(u3)," \
    "UAV(u4)"

cbuffer SimulateCB : register(b0)
{
    float g_dt;
    float g_gravity_y;
    float g_drag;
    float g_turbulence;
    uint  g_frame_counter;
    uint  g_pad0;
    uint  g_pad1;
    uint  g_pad2;
};

#include "particle_common.hlsli"

RWStructuredBuffer<Particle>        g_pool          : register(u0);
RWStructuredBuffer<uint>            g_alive_current : register(u1);
RWStructuredBuffer<uint>            g_alive_next    : register(u2);
RWStructuredBuffer<uint>            g_dead          : register(u3);
RWStructuredBuffer<ParticleCounter> g_counter       : register(u4);

[RootSignature(PARTICLE_SIMULATE_RS)]
[numthreads(64, 1, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    const uint i = dtid.x;
    if (i >= g_counter[0].alive_count_current) return;

    const uint pool_idx = g_alive_current[i];
    Particle p = g_pool[pool_idx];

    //~ early out if the slot is already dead this catches a stale alive entry
    //~ from a partial emit the dead flow already prevents it but the check is
    //~ cheap and bullet proof
    if (p.life <= 0.0)
    {
        uint dead_slot;
        InterlockedAdd(g_counter[0].dead_count, 1u, dead_slot);
        g_dead[dead_slot] = pool_idx;
        return;
    }

    //~ integrating velocity gravity for fire is an upward buoyancy push not a
    //~ pull the cpu passes a negative g_gravity_y so positive y rises spray sea
    //~ splash instead falls under real gravity so a droplet off a wave crest
    //~ arcs back down into the sea
    const float gy = (p.emitter_type == 2u) ? -9.8 : g_gravity_y;
    p.velocity.y += gy * g_dt;

    //~ curl noise turbulence a divergence free field per particle the time
    //~ offset scrolls it so the flame silhouette wobbles naturally nearby
    //~ particles see correlated motion which reads as one air mass moving
    const float t_off = float(g_frame_counter) * g_dt * 0.5;
    const float3 curl = particle_curl_noise(p.position * 0.6, t_off);
    p.velocity += curl * g_turbulence * g_dt;

    //~ drag a simple exponential bleed per dt so a long lived particle does not
    //~ pile noise up into infinity
    p.velocity *= saturate(1.0 - g_drag * g_dt);

    p.position += p.velocity * g_dt;
    p.life     -= g_dt;

    if (p.life <= 0.0)
    {
        //~ life ran out hand the slot back to the dead list the pool stays put
        //~ the next emit that pops this index overwrites it
        uint dead_slot;
        InterlockedAdd(g_counter[0].dead_count, 1u, dead_slot);
        g_dead[dead_slot] = pool_idx;
        return; //~ do not append to alive_next it is dead
    }

    //~ still alive write the particle back and append to alive_next
    g_pool[pool_idx] = p;

    uint slot;
    InterlockedAdd(g_counter[0].alive_count_next, 1u, slot);
    g_alive_next[slot] = pool_idx;
}
