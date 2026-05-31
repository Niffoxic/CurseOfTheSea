#define PARTICLE_RESET_RS \
    "UAV(u0)"

#include "particle_common.hlsli"

RWStructuredBuffer<ParticleCounter> g_counter : register(u0);

[RootSignature(PARTICLE_RESET_RS)]
[numthreads(1, 1, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x != 0) return;
    //~ survivors roll into current emit appends here and simulate reads here
    //~ then next resets so simulates InterlockedAdd starts from zero
    const uint survivors = g_counter[0].alive_count_next;
    g_counter[0].alive_count_current = survivors;
    g_counter[0].alive_count_next    = 0;
}
