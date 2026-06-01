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
#include "trishul/renderer/mesh/primitives.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <unordered_map>

namespace trishul::render::mesh::primitives
{
    namespace
    {
        constexpr float k_pi  = std::numbers::pi_v<float>;
        constexpr float k_tau = 2.0f * k_pi;

        [[nodiscard]] DirectX::XMFLOAT3 normalize(const DirectX::XMFLOAT3 v) noexcept
        {
            const float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
            if (len <= 1e-8f) return { 0.f, 0.f, 0.f };
            const float inv = 1.0f / len;
            return { v.x * inv, v.y * inv, v.z * inv };
        }

        //~ stamp the shared bits one submesh over the whole index range default
        //~ material slot zero then drop in the bounds
        void finalize(mesh_data& m, const char* name)
        {
            m.format = mesh_format::positions | mesh_format::normals | mesh_format::uvs;
            m.name   = name;
            m.submeshes.clear();
            m.submeshes.push_back(submesh{ 0u, m.index_count(), 0u, {} });
            m.recompute_bounds();
        }

        void push_vertex(mesh_data& m,
                         const DirectX::XMFLOAT3& p,
                         const DirectX::XMFLOAT3& n,
                         const DirectX::XMFLOAT2& uv)
        {
            m.positions.push_back(p);
            m.normals  .push_back(n);
            m.uvs      .push_back(uv);
        }
    } // anonymous namespace

    mesh_data cube(const float extent)
    {
        mesh_data m;
        const float h = extent * 0.5f;

        //~ six faces four verts each so each face keeps its own flat normal and
        //~ a clean 0..1 uv square
        struct face { DirectX::XMFLOAT3 n, u, v; };
        const face faces[6] =
        {
            { {  0,  0,  1 }, {  1, 0, 0 }, { 0, 1, 0 } }, //~ +z
            { {  0,  0, -1 }, { -1, 0, 0 }, { 0, 1, 0 } }, //~ -z
            { {  1,  0,  0 }, {  0, 0,-1 }, { 0, 1, 0 } }, //~ +x
            { { -1,  0,  0 }, {  0, 0, 1 }, { 0, 1, 0 } }, //~ -x
            { {  0,  1,  0 }, {  1, 0, 0 }, { 0, 0,-1 } }, //~ +y
            { {  0, -1,  0 }, {  1, 0, 0 }, { 0, 0, 1 } }, //~ -y
        };

        for (const face& f : faces)
        {
            const std::uint32_t base = m.vertex_count();
            //~ four corners centre +- the two in plane axes scaled to the half size
            const DirectX::XMFLOAT3 c{ f.n.x * h, f.n.y * h, f.n.z * h };
            const DirectX::XMFLOAT2 uvs[4] = { {0,1},{1,1},{1,0},{0,0} };
            const float signs[4][2] = { {-1,-1},{1,-1},{1,1},{-1,1} };
            for (int i = 0; i < 4; ++i)
            {
                const DirectX::XMFLOAT3 p{
                    c.x + (f.u.x * signs[i][0] + f.v.x * signs[i][1]) * h,
                    c.y + (f.u.y * signs[i][0] + f.v.y * signs[i][1]) * h,
                    c.z + (f.u.z * signs[i][0] + f.v.z * signs[i][1]) * h };
                push_vertex(m, p, f.n, uvs[i]);
            }
            m.indices.insert(m.indices.end(),
                { base + 0, base + 1, base + 2, base + 0, base + 2, base + 3 });
        }

        finalize(m, "cube");
        return m;
    }

    mesh_data plane(const float size, const std::uint32_t subdivisions)
    {
        mesh_data m;
        const std::uint32_t n = subdivisions + 1u; //~ verts per side
        const float half = size * 0.5f;
        const float step = (n > 1u) ? size / static_cast<float>(subdivisions) : size;

        for (std::uint32_t z = 0; z < n; ++z)
            for (std::uint32_t x = 0; x < n; ++x)
            {
                const float px = -half + static_cast<float>(x) * step;
                const float pz = -half + static_cast<float>(z) * step;
                const DirectX::XMFLOAT2 uv{
                    static_cast<float>(x) / static_cast<float>(subdivisions),
                    static_cast<float>(z) / static_cast<float>(subdivisions) };
                push_vertex(m, { px, 0.f, pz }, { 0.f, 1.f, 0.f }, uv);
            }

        for (std::uint32_t z = 0; z < subdivisions; ++z)
            for (std::uint32_t x = 0; x < subdivisions; ++x)
            {
                const std::uint32_t i0 = z * n + x;
                const std::uint32_t i1 = i0 + 1u;
                const std::uint32_t i2 = i0 + n;
                const std::uint32_t i3 = i2 + 1u;
                m.indices.insert(m.indices.end(), { i0, i2, i1, i1, i2, i3 });
            }

        finalize(m, "plane");
        return m;
    }

    mesh_data uv_sphere(const float radius, std::uint32_t rings, std::uint32_t sectors)
    {
        mesh_data m;
        if (rings   < 2u) rings   = 2u;
        if (sectors < 3u) sectors = 3u;

        for (std::uint32_t r = 0; r <= rings; ++r)
        {
            const float v   = static_cast<float>(r) / static_cast<float>(rings);
            const float phi = v * k_pi; //~ 0 at the top pole down to pi
            const float y   = std::cos(phi);
            const float rs  = std::sin(phi);
            for (std::uint32_t s = 0; s <= sectors; ++s)
            {
                const float u     = static_cast<float>(s) / static_cast<float>(sectors);
                const float theta = u * k_tau;
                const DirectX::XMFLOAT3 nrm{ rs * std::cos(theta), y, rs * std::sin(theta) };
                push_vertex(m, { nrm.x * radius, nrm.y * radius, nrm.z * radius },
                            nrm, { u, v });
            }
        }

        const std::uint32_t stride = sectors + 1u;
        for (std::uint32_t r = 0; r < rings; ++r)
            for (std::uint32_t s = 0; s < sectors; ++s)
            {
                const std::uint32_t i0 = r * stride + s;
                const std::uint32_t i1 = i0 + 1u;
                const std::uint32_t i2 = i0 + stride;
                const std::uint32_t i3 = i2 + 1u;
                m.indices.insert(m.indices.end(), { i0, i2, i1, i1, i2, i3 });
            }

        finalize(m, "uv_sphere");
        return m;
    }

    mesh_data ico_sphere(const float radius, const std::uint32_t subdivisions)
    {
        mesh_data m;

        //~ start from the twelve icosahedron verts then split each triangle into
        //~ four caching midpoints so shared edges stay welded
        const float t = (1.0f + std::sqrt(5.0f)) * 0.5f;
        std::vector<DirectX::XMFLOAT3> verts =
        {
            {-1, t, 0}, {1, t, 0}, {-1,-t, 0}, {1,-t, 0},
            {0,-1, t}, {0, 1, t}, {0,-1,-t}, {0, 1,-t},
            { t, 0,-1}, { t, 0, 1}, {-t, 0,-1}, {-t, 0, 1},
        };
        for (auto& v : verts) v = normalize(v);

        std::vector<std::uint32_t> tris =
        {
            0,11,5, 0,5,1, 0,1,7, 0,7,10, 0,10,11,
            1,5,9, 5,11,4, 11,10,2, 10,7,6, 7,1,8,
            3,9,4, 3,4,2, 3,2,6, 3,6,8, 3,8,9,
            4,9,5, 2,4,11, 6,2,10, 8,6,7, 9,8,1,
        };

        std::unordered_map<std::uint64_t, std::uint32_t> midpoint;
        auto mid = [&](std::uint32_t a, std::uint32_t b) -> std::uint32_t
        {
            const std::uint64_t key = (static_cast<std::uint64_t>(std::min(a, b)) << 32)
                                    |  static_cast<std::uint64_t>(std::max(a, b));
            if (const auto it = midpoint.find(key); it != midpoint.end()) return it->second;
            const DirectX::XMFLOAT3 m2 = normalize({
                (verts[a].x + verts[b].x) * 0.5f,
                (verts[a].y + verts[b].y) * 0.5f,
                (verts[a].z + verts[b].z) * 0.5f });
            const auto idx = static_cast<std::uint32_t>(verts.size());
            verts.push_back(m2);
            midpoint.emplace(key, idx);
            return idx;
        };

        for (std::uint32_t s = 0; s < subdivisions; ++s)
        {
            std::vector<std::uint32_t> next;
            next.reserve(tris.size() * 4u);
            for (std::size_t i = 0; i + 2 < tris.size() + 1; i += 3)
            {
                const std::uint32_t a = tris[i], b = tris[i + 1], c = tris[i + 2];
                const std::uint32_t ab = mid(a, b), bc = mid(b, c), ca = mid(c, a);
                next.insert(next.end(), { a, ab, ca,  b, bc, ab,  c, ca, bc,  ab, bc, ca });
            }
            tris.swap(next);
        }

        //~ a unit sphere so the position is also the normal uv from the spherical
        //~ angles seam wraps are acceptable for a debug primitive
        m.positions.reserve(verts.size());
        for (const auto& v : verts)
        {
            const float u = 0.5f + std::atan2(v.z, v.x) / k_tau;
            const float vv = 0.5f - std::asin(v.y) / k_pi;
            push_vertex(m, { v.x * radius, v.y * radius, v.z * radius }, v, { u, vv });
        }
        m.indices = std::move(tris);

        finalize(m, "ico_sphere");
        return m;
    }

    mesh_data cylinder(const float radius, const float height, std::uint32_t segments)
    {
        mesh_data m;
        if (segments < 3u) segments = 3u;
        const float h = height * 0.5f;

        //~ side wall a ring at the bottom and one at the top duplicated per
        //~ segment edge so the uvs run cleanly around
        for (std::uint32_t s = 0; s <= segments; ++s)
        {
            const float u     = static_cast<float>(s) / static_cast<float>(segments);
            const float theta = u * k_tau;
            const float cx = std::cos(theta), cz = std::sin(theta);
            const DirectX::XMFLOAT3 nrm{ cx, 0.f, cz };
            push_vertex(m, { cx * radius, -h, cz * radius }, nrm, { u, 1.f });
            push_vertex(m, { cx * radius,  h, cz * radius }, nrm, { u, 0.f });
        }
        for (std::uint32_t s = 0; s < segments; ++s)
        {
            const std::uint32_t i0 = s * 2u;
            const std::uint32_t i1 = i0 + 1u;
            const std::uint32_t i2 = i0 + 2u;
            const std::uint32_t i3 = i0 + 3u;
            m.indices.insert(m.indices.end(), { i0, i2, i1, i1, i2, i3 });
        }

        //~ caps each gets a centre fan top normal up bottom normal down
        auto cap = [&](const float y, const DirectX::XMFLOAT3 nrm, const bool flip)
        {
            const std::uint32_t centre = m.vertex_count();
            push_vertex(m, { 0.f, y, 0.f }, nrm, { 0.5f, 0.5f });
            const std::uint32_t ring = m.vertex_count();
            for (std::uint32_t s = 0; s <= segments; ++s)
            {
                const float theta = static_cast<float>(s) / static_cast<float>(segments) * k_tau;
                const float cx = std::cos(theta), cz = std::sin(theta);
                push_vertex(m, { cx * radius, y, cz * radius }, nrm,
                            { cx * 0.5f + 0.5f, cz * 0.5f + 0.5f });
            }
            for (std::uint32_t s = 0; s < segments; ++s)
            {
                const std::uint32_t a = ring + s;
                const std::uint32_t b = ring + s + 1u;
                if (flip) m.indices.insert(m.indices.end(), { centre, b, a });
                else      m.indices.insert(m.indices.end(), { centre, a, b });
            }
        };
        cap(  h, {  0.f, 1.f, 0.f }, false);
        cap( -h, {  0.f,-1.f, 0.f }, true);

        finalize(m, "cylinder");
        return m;
    }

    mesh_data cone(const float radius, const float height, std::uint32_t segments)
    {
        mesh_data m;
        if (segments < 3u) segments = 3u;
        const float h = height * 0.5f;

        //~ side apex duplicated per segment so each sloped face gets its own
        //~ normal pointing out and slightly up
        const DirectX::XMFLOAT3 apex{ 0.f, h, 0.f };
        const float slope = radius / (height > 1e-6f ? height : 1e-6f);
        for (std::uint32_t s = 0; s < segments; ++s)
        {
            const float t0 = static_cast<float>(s)        / static_cast<float>(segments) * k_tau;
            const float t1 = static_cast<float>(s + 1u)   / static_cast<float>(segments) * k_tau;
            const float tm = (t0 + t1) * 0.5f;
            const DirectX::XMFLOAT3 n0 = normalize({ std::cos(t0), slope, std::sin(t0) });
            const DirectX::XMFLOAT3 n1 = normalize({ std::cos(t1), slope, std::sin(t1) });
            const DirectX::XMFLOAT3 na = normalize({ std::cos(tm), slope, std::sin(tm) });
            const std::uint32_t base = m.vertex_count();
            push_vertex(m, apex, na, { 0.5f, 0.f });
            push_vertex(m, { std::cos(t0) * radius, -h, std::sin(t0) * radius }, n0, { 0.f, 1.f });
            push_vertex(m, { std::cos(t1) * radius, -h, std::sin(t1) * radius }, n1, { 1.f, 1.f });
            m.indices.insert(m.indices.end(), { base, base + 1u, base + 2u });
        }

        //~ base cap fan facing down
        const std::uint32_t centre = m.vertex_count();
        push_vertex(m, { 0.f, -h, 0.f }, { 0.f, -1.f, 0.f }, { 0.5f, 0.5f });
        const std::uint32_t ring = m.vertex_count();
        for (std::uint32_t s = 0; s <= segments; ++s)
        {
            const float theta = static_cast<float>(s) / static_cast<float>(segments) * k_tau;
            const float cx = std::cos(theta), cz = std::sin(theta);
            push_vertex(m, { cx * radius, -h, cz * radius }, { 0.f, -1.f, 0.f },
                        { cx * 0.5f + 0.5f, cz * 0.5f + 0.5f });
        }
        for (std::uint32_t s = 0; s < segments; ++s)
            m.indices.insert(m.indices.end(), { centre, ring + s + 1u, ring + s });

        finalize(m, "cone");
        return m;
    }

    mesh_data capsule(const float radius, const float body_height,
                      std::uint32_t segments, std::uint32_t rings)
    {
        mesh_data m;
        if (segments < 3u) segments = 3u;
        if (rings    < 1u) rings    = 1u;
        const float h = body_height * 0.5f; //~ half of the straight middle

        //~ build it as latitude rings top hemisphere then bottom each ring sits
        //~ at radius around the axis and the centre y is pushed out by +-h so the
        //~ two caps are separated by the cylinder body sharing the side verts
        const std::uint32_t half_rings = rings;
        auto ring_row = [&](const float phi, const float y_off)
        {
            const float sp = std::sin(phi), cp = std::cos(phi);
            for (std::uint32_t s = 0; s <= segments; ++s)
            {
                const float u     = static_cast<float>(s) / static_cast<float>(segments);
                const float theta = u * k_tau;
                const DirectX::XMFLOAT3 nrm{ sp * std::cos(theta), cp, sp * std::sin(theta) };
                push_vertex(m,
                    { nrm.x * radius, nrm.y * radius + y_off, nrm.z * radius },
                    nrm, { u, (cp * 0.5f + 0.5f) });
            }
        };

        //~ top cap phi 0..pi/2 offset +h
        for (std::uint32_t r = 0; r <= half_rings; ++r)
            ring_row((static_cast<float>(r) / static_cast<float>(half_rings)) * (k_pi * 0.5f), h);
        //~ bottom cap phi pi/2..pi offset -h
        for (std::uint32_t r = 0; r <= half_rings; ++r)
            ring_row(k_pi * 0.5f + (static_cast<float>(r) / static_cast<float>(half_rings)) * (k_pi * 0.5f), -h);

        const std::uint32_t stride    = segments + 1u;
        const std::uint32_t total_rows = (half_rings + 1u) * 2u;
        for (std::uint32_t r = 0; r + 1u < total_rows; ++r)
            for (std::uint32_t s = 0; s < segments; ++s)
            {
                const std::uint32_t i0 = r * stride + s;
                const std::uint32_t i1 = i0 + 1u;
                const std::uint32_t i2 = i0 + stride;
                const std::uint32_t i3 = i2 + 1u;
                m.indices.insert(m.indices.end(), { i0, i2, i1, i1, i2, i3 });
            }

        finalize(m, "capsule");
        return m;
    }
} // namespace trishul::render::mesh::primitives
