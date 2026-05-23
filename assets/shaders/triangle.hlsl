// Created by Niffoxic (Harsh Dubey)
// triangle with SoA vertex inputs (test only)
#define COTS_RS "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)"

struct VSInput
{
    float3 position : POSITION;   // stream
    float3 color    : COLOR;
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 color    : COLOR;
};

[RootSignature(COTS_RS)]
VSOutput VSMain(VSInput input)
{
    VSOutput o;
    o.position = float4(input.position, 1.0);
    o.color    = input.color;
    return o;
}

float4 PSMain(VSOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}
