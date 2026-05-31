#ifndef PARTICLE_COMMON_HLSLI
#define PARTICLE_COMMON_HLSLI

//~ the shared particle structs and helpers everyone here sees the same aos
//~ stride init emit simulate render the c++ side static_asserts these against
//~ sizeof so if one side drifts the build catches it

struct Particle
{
    float3 position;     //~ 12
    float  life;         //~  4 to 16
    float3 velocity;     //~ 12
    float  max_life;     //~  4 to 32
    float  size;         //~  4
    uint   color_seed;   //~  4
    uint   emitter_type; //~  4  matches particle_type 0 fire 1 smoke 2 spray
    uint   pad1;         //~  4 to 48
};

struct ParticleCounter
{
    uint dead_count;
    uint alive_count_current;
    uint alive_count_next;
    uint pad0;
};

struct ParticleEmitter
{
    uint   type;
    float3 position;
    float  spawn_radius;
    float  min_lifetime;
    float  max_lifetime;
    float  start_size;
    uint   base_color;
    uint   spawn_count;
    uint   first_thread;
    uint   pad0;
};

//~ a pcg style 32 bit hash emit and simulate fold frame thread and emitter ids
//~ through this for a per particle rng seed so we never need a noise texture
uint particle_hash_u32(uint x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

//~ a 0..1 float out of a hash deterministic per seed xor a salt in to pull
//~ several independent floats from one seed
float particle_rand01(uint seed)
{
    return float(particle_hash_u32(seed) & 0x00ffffffu) / float(0x01000000u);
}

float particle_rand_range(uint seed, float lo, float hi)
{
    return lo + (hi - lo) * particle_rand01(seed);
}

//~ packing a 0..1 float4 colour into a uint the emit shader stashes the start
//~ colour here the renderer unpacks it the same way standard r8g8b8a8 little
//~ endian ordering
uint particle_pack_unorm_color(float4 c)
{
    uint r = uint(saturate(c.r) * 255.0 + 0.5);
    uint g = uint(saturate(c.g) * 255.0 + 0.5);
    uint b = uint(saturate(c.b) * 255.0 + 0.5);
    uint a = uint(saturate(c.a) * 255.0 + 0.5);
    return (a << 24) | (b << 16) | (g << 8) | r;
}

float4 particle_unpack_unorm_color(uint c)
{
    float r = float( c        & 0xffu) / 255.0;
    float g = float((c >>  8) & 0xffu) / 255.0;
    float b = float((c >> 16) & 0xffu) / 255.0;
    float a = float((c >> 24) & 0xffu) / 255.0;
    return float4(r, g, b, a);
}

//~ cheap 3d value noise via hashed corners interpolated the curl variant below
//~ builds a divergence free flow out of it so the simulate can ride convincing
//~ turbulence without a noise texture
float particle_value_noise3(float3 p)
{
    const float3 fp = floor(p);
    const float3 lp = p - fp;
    const float3 s  = lp * lp * (3.0 - 2.0 * lp);

    //~ corner hashes reuse the same pcg mixer the pool rng rides so the noise
    //~ stays internally consistent
    const uint hx = uint(fp.x);
    const uint hy = uint(fp.y);
    const uint hz = uint(fp.z);

    float c000 = particle_rand01(particle_hash_u32(hx + 73u*hy + 19349663u*hz));
    float c100 = particle_rand01(particle_hash_u32((hx+1u) + 73u*hy + 19349663u*hz));
    float c010 = particle_rand01(particle_hash_u32(hx + 73u*(hy+1u) + 19349663u*hz));
    float c110 = particle_rand01(particle_hash_u32((hx+1u) + 73u*(hy+1u) + 19349663u*hz));
    float c001 = particle_rand01(particle_hash_u32(hx + 73u*hy + 19349663u*(hz+1u)));
    float c101 = particle_rand01(particle_hash_u32((hx+1u) + 73u*hy + 19349663u*(hz+1u)));
    float c011 = particle_rand01(particle_hash_u32(hx + 73u*(hy+1u) + 19349663u*(hz+1u)));
    float c111 = particle_rand01(particle_hash_u32((hx+1u) + 73u*(hy+1u) + 19349663u*(hz+1u)));

    const float n00 = lerp(c000, c100, s.x);
    const float n10 = lerp(c010, c110, s.x);
    const float n01 = lerp(c001, c101, s.x);
    const float n11 = lerp(c011, c111, s.x);

    const float n0  = lerp(n00, n10, s.y);
    const float n1  = lerp(n01, n11, s.y);

    return lerp(n0, n1, s.z) * 2.0 - 1.0;
}

//~ a divergence free vector field the curl of three uncorrelated scalar fields
//~ done with central difference gradients the time offset scrolls the field so
//~ the turbulence drifts over time simulate uses this as a per dt velocity bump
float3 particle_curl_noise(float3 p, float time_offset)
{
    const float eps = 0.1;
    const float3 ox = float3(eps, 0.0, 0.0);
    const float3 oy = float3(0.0, eps, 0.0);
    const float3 oz = float3(0.0, 0.0, eps);

    //~ three potential fields keyed by separate offsets so their gradients do
    //~ not correlate the time offset rolls the whole thing forward
    const float3 px = p + float3(31.0, 0.0, 0.0) + time_offset;
    const float3 py = p + float3(0.0, 31.0, 0.0) + time_offset;
    const float3 pz = p + float3(0.0, 0.0, 31.0) + time_offset;

    const float dax = (particle_value_noise3(pz + oy) -
                       particle_value_noise3(pz - oy)) / (2.0 * eps);
    const float day = (particle_value_noise3(pz + ox) -
                       particle_value_noise3(pz - ox)) / (2.0 * eps);

    const float dbx = (particle_value_noise3(py + oz) -
                       particle_value_noise3(py - oz)) / (2.0 * eps);
    const float dbz = (particle_value_noise3(py + ox) -
                       particle_value_noise3(py - ox)) / (2.0 * eps);

    const float dcy = (particle_value_noise3(px + oz) -
                       particle_value_noise3(px - oz)) / (2.0 * eps);
    const float dcz = (particle_value_noise3(px + oy) -
                       particle_value_noise3(px - oy)) / (2.0 * eps);

    return float3(dcy - dbz, dax - dcz, dbx - day);
}

//~ the fire colour ramp age 0 spawn 1 death white to yellow to orange to red
//~ both the emit start colour and the pixel shader tint pull from here so the
//~ flame reads right without any artist tuning
float3 particle_fire_color(float t)
{
    t = saturate(t);
    //~ four stops three segments lerped piece wise on t
    const float3 c_white  = float3(1.5, 1.5, 1.2);  //~ overbright spawn
    const float3 c_yellow = float3(1.4, 1.0, 0.2);
    const float3 c_orange = float3(1.2, 0.45, 0.05);
    const float3 c_red    = float3(0.8, 0.10, 0.02);

    float3 col;
    if (t < 0.25)
    {
        col = lerp(c_white,  c_yellow, t / 0.25);
    }
    else if (t < 0.55)
    {
        col = lerp(c_yellow, c_orange, (t - 0.25) / 0.30);
    }
    else
    {
        col = lerp(c_orange, c_red,    (t - 0.55) / 0.45);
    }
    return col;
}

//~ the smoke colour ramp warm grey near spawn drifting cooler and darker as it
//~ ages it stays under one in linear hdr since smoke reads as alpha blended not
//~ additive the renderer premultiplies by alpha so the blend equation lines up
float3 particle_smoke_color(float t)
{
    t = saturate(t);
    const float3 c_warm = float3(0.50, 0.48, 0.46);  //~ young smoke
    const float3 c_cool = float3(0.18, 0.19, 0.22);  //~ aged smoke
    return lerp(c_warm, c_cool, t);
}

#endif //~ PARTICLE_COMMON_HLSLI
