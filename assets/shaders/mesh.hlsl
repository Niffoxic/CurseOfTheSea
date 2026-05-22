#include "cots_prelude.hlsli"

//~ cpu writes are row major
cbuffer FrameCB : register(b0)
{
    row_major float4x4 g_view_proj;
};

cbuffer ObjectCB : register(b1)
{
    row_major float4x4 g_world;
    uint  g_material;   //~ holds the bindless texture slot for now
    uint3 g_pad;
};

struct VSIn
{
    float3 position : POSITION;
    float3 color    : COLOR;
    float2 uv       : TEXCOORD;
};

struct VSOut
{
    float4 sv_position : SV_Position;
    float3 color       : COLOR;
    float2 uv          : TEXCOORD;
};

VSOut VSMain(VSIn input)
{
    VSOut output;
    const float4 world_pos = mul(float4(input.position, 1.0f), g_world);
    output.sv_position     = mul(world_pos, g_view_proj);
    output.color           = input.color;
    output.uv              = input.uv;
    return output;
}

float4 PSMain(VSOut input) : SV_Target
{
    //~ index the bindless heap to grab the sampleable srv
    Texture2D tex = ResourceDescriptorHeap[g_material];

    //~ swap to g point wrap to compare point versus linear filtering
    return tex.Sample(g_linear_wrap, input.uv);
}
