#define PARTICLE_BUILD_SIM_ARGS_RS \
    "UAV(u0)," \
    "UAV(u1)"

#include "particle_common.hlsli"

RWStructuredBuffer<ParticleCounter> g_counter : register(u0);

struct DispatchArgs
{
    uint thread_groups_x;
    uint thread_groups_y;
    uint thread_groups_z;
};
RWStructuredBuffer<DispatchArgs> g_args : register(u1);

[RootSignature(PARTICLE_BUILD_SIM_ARGS_RS)]
[numthreads(1, 1, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x != 0) return;
    //~ one group per 64 alive particles rounded up
    const uint alive_n = g_counter[0].alive_count_current;
    g_args[0].thread_groups_x = (alive_n + 63) / 64;
    g_args[0].thread_groups_y = 1;
    g_args[0].thread_groups_z = 1;
}
