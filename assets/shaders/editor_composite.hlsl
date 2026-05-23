#include "cots_prelude.hlsli"

//~ root constants set per draw
//  x = bindless slot of the editor UI BGRA texture
cbuffer EditorComposite : register(b0)
{
    uint  g_editor_ui_slot;
    uint3 g_pad;
};

struct VSOut
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
};

//~ classic fullscreen triangle no vertex buffer
VSOut VSMain(uint vid : SV_VertexID)
{
    float2 uv  = float2((vid << 1) & 2, vid & 2);
    VSOut  o;
    o.uv  = uv;
    o.pos = float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return o;
}

float4 PSMain(VSOut input) : SV_Target0
{
    //~ Ultralight surfaces are BGRA premultiplied alpha top left
    // the texture format is B8G8R8A8_UNORM_SRGB so the GPU returns logical
    // RGBA already swizzled, just sample and pass through
    Texture2D<float4> ui = ResourceDescriptorHeap[g_editor_ui_slot];
    float4 src = ui.Sample(g_linear_clamp, input.uv);

    //~ alpha blend is configured as ONE / INV_SRC_ALPHA in the PSO so
    //  premultiplied src composites correctly over the existing color
    return src;
}
