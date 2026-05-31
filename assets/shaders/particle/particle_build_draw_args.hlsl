#define PARTICLE_BUILD_DRAW_ARGS_RS \
    "UAV(u0)," \
    "UAV(u1)"

#include "particle_common.hlsli"

RWStructuredBuffer<ParticleCounter> g_counter : register(u0);

struct DrawArgs
{
    uint vertex_count_per_instance;
    uint instance_count;
    uint start_vertex_location;
    uint start_instance_location;
};
RWStructuredBuffer<DrawArgs> g_args : register(u1);

[RootSignature(PARTICLE_BUILD_DRAW_ARGS_RS)]
[numthreads(1, 1, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x != 0) return;
    //~ six verts per particle two triangles instance count is the survivor total
    g_args[0].vertex_count_per_instance = 6;
    g_args[0].instance_count            = g_counter[0].alive_count_next;
    g_args[0].start_vertex_location     = 0;
    g_args[0].start_instance_location   = 0;
}
