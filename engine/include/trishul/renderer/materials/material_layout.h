//=============================================================================
// Curse of the Sea
//=============================================================================
// Created by  Niffoxic - Harsh Dubey
// Module      WM9M6 Fundamentals of Games Research Development and Management
// Institution University of Warwick
//
// A linear story driven pirate adventure built from scratch in C++23 and
// DirectX 12 for the University of Warwick game project assessment.
//=============================================================================
#ifndef CURSEOFTHESEA_MATERIAL_LAYOUT_H
#define CURSEOFTHESEA_MATERIAL_LAYOUT_H

#include <array>
#include <cstdint>
#include <dxgiformat.h>

namespace trishul::render::materials
{
    //~ engine wide material limits these are a shader contract every material
    //~ shader has to agree on so nudging them means bumping the shaders too
    constexpr std::uint32_t k_max_texture_slots = 8u;
    constexpr std::uint32_t k_max_param_bytes   = 128u;

    //~ the per material constant buffer we upload first the eight bindless slot
    //~ indices then 128 opaque param bytes the shader reads whatever cbuffer
    //~ struct it wants out of this 160 bytes fits comfortably inside 256 align
    struct gpu_material_cb
    {
        std::uint32_t slots [k_max_texture_slots] { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u };
        std::uint8_t  params[k_max_param_bytes]   {};
    };
    static_assert(sizeof(gpu_material_cb) == 160u,
                  "gpu_material_cb is the shader contract bump the version on any change");

    //~ the canonical engine vertex layout every material shader consumes some
    //~ subset of these extra shader side attributes are not supported this phase
    struct vertex_attr
    {
        const char*    semantic_name;
        std::uint32_t  semantic_index;
        DXGI_FORMAT    format;
        std::uint32_t  input_slot;
    };

    //~ NORMAL rides in the canonical set now the default mesh shader samples
    //~ slot 1 as a normal map so every mesh carries a NORMAL stream importers
    //~ fall back to flat per face normals when the source has none so it holds
    inline constexpr std::array<vertex_attr, 4> k_canonical_vertex_attrs = {{
        { "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u },
        { "COLOR",    0u, DXGI_FORMAT_R32G32B32_FLOAT, 1u },
        { "TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT,    2u },
        { "NORMAL",   0u, DXGI_FORMAT_R32G32B32_FLOAT, 3u },
    }};

    //~ root parameter slots for the stable material root signature b1 is a one
    //~ uint root constant naming the visible base offset for the current
    //~ ExecuteIndirect draw the vs reads its instance index off the visible
    //~ buffer then loads the world matrix from the instance srv the command
    //~ signature sets b1 per indirect draw
    enum class root_param : std::uint32_t
    {
        frame_cb     = 0u,   //~ b0 FrameCB view projection plus lights
        draw_consts  = 1u,   //~ b1 root constants visible_base offset
        material_cb  = 2u,   //~ b2 MaterialCB slot indices and params
        instance_srv = 3u,   //~ t0 instance structured buffer root srv
        visible_srv  = 4u,   //~ t1 visible index buffer root srv
    };

    //~ one gpu instance per snapshot instance uploaded each frame to a
    //~ structured buffer the pack has to match HLSL StructuredBuffer<GpuInstance>
    //~ stride if either side drifts every draw lands on the wrong matrix world
    //~ is a full row major float4x4 dropping the fourth row would zero the
    //~ translation and pile everything at the origin the cull shader uses
    //~ center_radius for a quick sphere reject then aabb_half_extent for the
    //~ tight test bucket_id names the ExecuteIndirect bucket this instance
    //~ shares with the rest of its mesh material pair
    struct gpu_instance
    {
        float          world_r0[4];     //~ row 0 basis x
        float          world_r1[4];     //~ row 1 basis y
        float          world_r2[4];     //~ row 2 basis z
        float          world_r3[4];     //~ row 3 translation
        float          center_radius[4];//~ xyz centre w radius
        float          aabb_half_extent[3]; //~ half extent
        std::uint32_t  bucket_id;       //~ draw bucket index
    };
    static_assert(sizeof(gpu_instance) == 96u,
                  "gpu_instance layout must match cull.hlsl and mesh.hlsl");

    //~ one per draw indirect args record matching the engine command signature
    //~ a root uint for visible_base then the standard draw indexed args the cpu
    //~ fills the static fields at frame build time the gpu cull writes only
    //~ instance_count through an InterlockedAdd
    struct gpu_draw_args
    {
        std::uint32_t  visible_base;             //~ root constant 1 uint
        std::uint32_t  index_count_per_instance; //~ DRAW_INDEXED arg 0
        std::uint32_t  instance_count;           //~ DRAW_INDEXED arg 1 gpu writes this
        std::uint32_t  start_index_location;     //~ DRAW_INDEXED arg 2
        std::int32_t   base_vertex_location;     //~ DRAW_INDEXED arg 3
        std::uint32_t  start_instance_location;  //~ DRAW_INDEXED arg 4
    };
    static_assert(sizeof(gpu_draw_args) == 24u,
                  "gpu_draw_args must mirror D3D12_DRAW_INDEXED_ARGUMENTS plus the base const");
} // namespace trishul::render::materials

#endif //CURSEOFTHESEA_MATERIAL_LAYOUT_H
