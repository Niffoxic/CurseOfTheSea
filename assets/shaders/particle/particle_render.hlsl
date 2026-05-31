#define PARTICLE_RENDER_RS \
    "RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED)," \
    "CBV(b0)," \
    "SRV(t0)," \
    "SRV(t1)," \
    "StaticSampler(s0, " \
        "filter = FILTER_MIN_MAG_MIP_LINEAR, " \
        "addressU = TEXTURE_ADDRESS_CLAMP, " \
        "addressV = TEXTURE_ADDRESS_CLAMP, " \
        "addressW = TEXTURE_ADDRESS_CLAMP)"

cbuffer ParticleFrameCB : register(b0)
{
    row_major float4x4 g_view_proj;
    float3             g_camera_right;
    float              g_screen_width;
    float3             g_camera_up;
    float              g_screen_height;
    uint               g_depth_srv;
    float              g_soft_range;
    float              g_pad0;
    float              g_pad1;
};

#include "particle_common.hlsli"

StructuredBuffer<Particle> g_pool        : register(t0);
StructuredBuffer<uint>     g_alive       : register(t1);

struct VSOut
{
    float4 sv_pos       : SV_Position;
    float2 uv           : TEXCOORD0;
    float4 color        : COLOR0;
    float  age_t        : COLOR1;
    float  particle_z   : COLOR2;
    nointerpolation uint emitter_type : COLOR3;
};

//~ the two triangles of the billboard quad as corner offsets and matching uvs
static const float2 k_corner_offsets[6] =
{
    float2(-0.5, -0.5),
    float2(+0.5, -0.5),
    float2(-0.5, +0.5),
    float2(+0.5, -0.5),
    float2(+0.5, +0.5),
    float2(-0.5, +0.5),
};
static const float2 k_corner_uvs[6] =
{
    float2(0.0, 1.0),
    float2(1.0, 1.0),
    float2(0.0, 0.0),
    float2(1.0, 1.0),
    float2(1.0, 0.0),
    float2(0.0, 0.0),
};

[RootSignature(PARTICLE_RENDER_RS)]
VSOut VSMain(uint vid : SV_VertexID, uint iid : SV_InstanceID)
{
    VSOut o;

    const uint pool_idx = g_alive[iid];
    const Particle p    = g_pool[pool_idx];

    //~ a dead slot folds the quad to a degenerate point behind the far plane so
    //~ it costs nothing and never shades
    if (p.life <= 0.0 || p.max_life <= 0.0)
    {
        o.sv_pos       = float4(0.0, 0.0, 2.0, 1.0);
        o.uv           = float2(0.0, 0.0);
        o.color        = float4(0.0, 0.0, 0.0, 0.0);
        o.age_t        = 1.0;
        o.particle_z   = 0.0;
        o.emitter_type = 0u;
        return o;
    }

    const float2 corner = k_corner_offsets[vid];
    const float2 uv     = k_corner_uvs[vid];

    //~ a per type size curve fire pulses up then collapses smoke just keeps
    //~ swelling so the puff visibly billows the lifetime cap bounds it so a long
    //~ lived puff does not balloon off screen
    const float age_t = saturate(1.0 - p.life / max(p.max_life, 1e-3));
    float size_mul;
    if (p.emitter_type == 1u) //~ smoke
    {
        size_mul = lerp(1.0, 3.0, age_t);
    }
    else //~ fire and spray
    {
        size_mul = lerp(1.0, 1.4, smoothstep(0.0, 0.3, age_t)) *
                   lerp(1.0, 0.4, smoothstep(0.6, 1.0, age_t));
    }

    //~ expanding the quad along the camera right and up so it always faces us
    const float3 world_offset =
        g_camera_right * corner.x * p.size * size_mul +
        g_camera_up    * corner.y * p.size * size_mul;

    const float3 world_pos = p.position + world_offset;
    o.sv_pos = mul(float4(world_pos, 1.0), g_view_proj);

    o.color        = particle_unpack_unorm_color(p.color_seed);
    o.uv           = uv;
    o.age_t        = age_t;
    //~ ndc z after the perspective divide the pixel shader compares this to the
    //~ scene depth for the soft fade reversed z so closer to camera reads higher
    o.particle_z   = o.sv_pos.z / max(o.sv_pos.w, 1e-6);
    o.emitter_type = p.emitter_type;
    return o;
}

float4 PSMain(VSOut input) : SV_Target
{
    //~ radial distance from the quad centre drives every falloff below
    const float2 d  = input.uv - float2(0.5, 0.5);
    const float  r2 = dot(d, d);

    //~ soft particle fade tap the bindless scene depth at this pixel and compare
    //~ to the particles ndc z reversed z so the near value is bigger a positive
    //~ diff means the particle sits in front the soft range sets how early the
    //~ fade kicks in a zero depth slot means no tap so the factor stays one
    float soft = 1.0;
    if (g_depth_srv != 0)
    {
        Texture2D<float> scene_depth = ResourceDescriptorHeap[g_depth_srv];
        const int3 px = int3(input.sv_pos.xy, 0);
        const float scene_z = scene_depth.Load(px).r;
        const float diff = input.particle_z - scene_z;
        soft = saturate(diff / max(g_soft_range, 1e-5));
    }

    if (input.emitter_type == 2u) //~ spray sea splash
    {
        //~ a tight round droplet tinted by the emitter colour foamy white blue
        //~ alpha fades over life and softens on the scene depth so spray
        //~ dissolves into the water premultiplied so it reads as real spray not
        //~ an additive glow the way fire does
        const float m     = saturate(1.0 - r2 * 4.0);
        const float a_age = (1.0 - input.age_t) * input.color.a;
        const float a     = saturate(m * m * a_age * soft);
        return float4(input.color.rgb * a, a);
    }

    if (input.emitter_type == 1u) //~ smoke
    {
        //~ a gaussian disk fading to zero at the quad edge so the billboard
        //~ never reads as a hard square alpha opens while young pools then fades
        //~ on the back end so a puff billows in and out without a pop
        const float  m     = exp(-r2 * 6.0);
        const float  open  = smoothstep(0.0, 0.20, input.age_t);
        const float  close = 1.0 - smoothstep(0.55, 1.0, input.age_t);
        const float  a_age = open * close;
        const float3 ramp  = particle_smoke_color(input.age_t);
        const float  a     = saturate(m * a_age * soft * 0.55);
        //~ premultiplied alpha so the blend equation reads it as a proper alpha
        //~ blend and a dense smoke column occludes the scene behind it
        return float4(ramp * a, a);
    }

    //~ fire white to yellow to orange to red premultiplied with alpha zero so
    //~ the blend equation degenerates to pure additive which is the look we want
    const float  m     = saturate(1.0 - r2 * 4.0);
    const float3 ramp  = particle_fire_color(input.age_t);
    const float  a_age = (1.0 - input.age_t) * input.color.a;
    const float  a     = m * m * a_age * soft;
    return float4(ramp * a, 0.0);
}
