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
#include "trishul/renderer/mesh/terrain.h"

#include <cmath>
#include <vector>

#include <FastNoise/FastNoise.h>

namespace trishul::render::mesh
{
    mesh_data generate_terrain(const terrain_params& params)
    {
        mesh_data m;

        const std::uint32_t res = params.resolution < 2u ? 2u : params.resolution;
        const float size   = params.world_size > 0.f ? params.world_size : 1.0f;
        const float half   = size * 0.5f;
        const float step   = size / static_cast<float>(res - 1u);

        auto simplex = FastNoise::New<FastNoise::Simplex>();
        auto fbm     = FastNoise::New<FastNoise::FractalFBm>();
        fbm->SetSource(simplex);
        fbm->SetOctaveCount(params.octaves);
        fbm->SetLacunarity(params.lacunarity);
        fbm->SetGain(params.gain);

        //~ one uniform grid pass fills the whole height field roughly minus one
        //~ to one which the height knob then scales
        std::vector<float> heights(static_cast<std::size_t>(res) * res);
        fbm->GenUniformGrid2D(heights.data(), 0, 0,
                              static_cast<int>(res), static_cast<int>(res),
                              params.frequency, static_cast<int>(params.seed));

        auto height_at = [&](const std::uint32_t c, const std::uint32_t r) -> float
        {
            return heights[static_cast<std::size_t>(r) * res + c] * params.height;
        };

        //~ positions in the xz plane centred on the origin y from the noise
        m.positions.reserve(static_cast<std::size_t>(res) * res);
        m.normals  .reserve(static_cast<std::size_t>(res) * res);
        m.uvs      .reserve(static_cast<std::size_t>(res) * res);
        for (std::uint32_t r = 0; r < res; ++r)
            for (std::uint32_t c = 0; c < res; ++c)
            {
                const float px = -half + static_cast<float>(c) * step;
                const float pz = -half + static_cast<float>(r) * step;
                m.positions.push_back({ px, height_at(c, r), pz });

                //~ central difference gradient for a smooth normal clamp at the
                //~ edges so the borders do not spike
                const std::uint32_t cl = c > 0u        ? c - 1u : c;
                const std::uint32_t cr = c + 1u < res  ? c + 1u : c;
                const std::uint32_t ru = r > 0u        ? r - 1u : r;
                const std::uint32_t rd = r + 1u < res  ? r + 1u : r;
                const float dhdx = (height_at(cr, r) - height_at(cl, r)) / (step * 2.0f);
                const float dhdz = (height_at(c, rd) - height_at(c, ru)) / (step * 2.0f);
                const float nx = -dhdx, ny = 1.0f, nz = -dhdz;
                const float inv = 1.0f / std::sqrt(nx * nx + ny * ny + nz * nz);
                m.normals.push_back({ nx * inv, ny * inv, nz * inv });

                m.uvs.push_back({
                    static_cast<float>(c) / static_cast<float>(res - 1u),
                    static_cast<float>(r) / static_cast<float>(res - 1u) });
            }

        m.indices.reserve(static_cast<std::size_t>(res - 1u) * (res - 1u) * 6u);
        for (std::uint32_t r = 0; r + 1u < res; ++r)
            for (std::uint32_t c = 0; c + 1u < res; ++c)
            {
                const std::uint32_t i0 = r * res + c;
                const std::uint32_t i1 = i0 + 1u;
                const std::uint32_t i2 = i0 + res;
                const std::uint32_t i3 = i2 + 1u;
                m.indices.insert(m.indices.end(), { i0, i2, i1, i1, i2, i3 });
            }

        m.format = mesh_format::positions | mesh_format::normals | mesh_format::uvs;
        m.name   = "terrain";
        m.submeshes.push_back(submesh{ 0u, m.index_count(), 0u, {} });
        m.recompute_bounds();
        return m;
    }
} // namespace trishul::render::mesh
