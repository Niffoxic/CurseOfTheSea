//~ mesh shader textured cube via sm six six bindless dynamic resources

#define MESH_ROOT_SIG \
    "RootFlags(" \
        "ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT|" \
        "CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED|" \
        "DENY_HULL_SHADER_ROOT_ACCESS|" \
        "DENY_DOMAIN_SHADER_ROOT_ACCESS|" \
        "DENY_GEOMETRY_SHADER_ROOT_ACCESS|" \
        "DENY_AMPLIFICATION_SHADER_ROOT_ACCESS|" \
        "DENY_MESH_SHADER_ROOT_ACCESS" \
    ")," \
    "CBV(b0)," \
    "CBV(b1)," \
    "StaticSampler(s0," \
        "filter=FILTER_MIN_MAG_MIP_POINT," \
        "addressU=TEXTURE_ADDRESS_WRAP," \
        "addressV=TEXTURE_ADDRESS_WRAP," \
        "addressW=TEXTURE_ADDRESS_WRAP)," \
    "StaticSampler(s1," \
        "filter=FILTER_MIN_MAG_MIP_LINEAR," \
        "addressU=TEXTURE_ADDRESS_WRAP," \
        "addressV=TEXTURE_ADDRESS_WRAP," \
        "addressW=TEXTURE_ADDRESS_WRAP)," \
    "StaticSampler(s2," \
        "filter=FILTER_ANISOTROPIC," \
        "addressU=TEXTURE_ADDRESS_WRAP," \
        "addressV=TEXTURE_ADDRESS_WRAP," \
        "addressW=TEXTURE_ADDRESS_WRAP," \
        "maxAnisotropy=8)," \
    "StaticSampler(s3," \
        "filter=FILTER_MIN_MAG_MIP_POINT," \
        "addressU=TEXTURE_ADDRESS_CLAMP," \
        "addressV=TEXTURE_ADDRESS_CLAMP," \
        "addressW=TEXTURE_ADDRESS_CLAMP)," \
    "StaticSampler(s4," \
        "filter=FILTER_MIN_MAG_MIP_LINEAR," \
        "addressU=TEXTURE_ADDRESS_CLAMP," \
        "addressV=TEXTURE_ADDRESS_CLAMP," \
        "addressW=TEXTURE_ADDRESS_CLAMP)," \
    "StaticSampler(s5," \
        "filter=FILTER_ANISOTROPIC," \
        "addressU=TEXTURE_ADDRESS_CLAMP," \
        "addressV=TEXTURE_ADDRESS_CLAMP," \
        "addressW=TEXTURE_ADDRESS_CLAMP," \
        "maxAnisotropy=8)"

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

SamplerState g_point_wrap   : register(s0);
SamplerState g_linear_wrap  : register(s1);
SamplerState g_aniso_wrap   : register(s2);
SamplerState g_point_clamp  : register(s3);
SamplerState g_linear_clamp : register(s4);
SamplerState g_aniso_clamp  : register(s5);

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

[RootSignature(MESH_ROOT_SIG)]
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
