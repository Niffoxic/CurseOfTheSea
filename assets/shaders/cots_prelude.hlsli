#ifndef COTS_PRELUDE_HLSLI
#define COTS_PRELUDE_HLSLI

//~ the engine static samplers every pass sees these on s0..s6 in the same order
//~ the root signature builder lays them down so one sampler name works anywhere
SamplerState           g_point_wrap   : register(s0);
SamplerState           g_linear_wrap  : register(s1);
SamplerState           g_aniso_wrap   : register(s2);
SamplerState           g_point_clamp  : register(s3);
SamplerState           g_linear_clamp : register(s4);
SamplerState           g_aniso_clamp  : register(s5);
//~ the shadow compare sampler giving hardware pcf on SampleCmpLevelZero the
//~ white border reads receivers past the cascade as lit and the clamp keeps
//~ stray taps inside the cascade extent
SamplerComparisonState g_shadow_cmp   : register(s6);

#endif //~ COTS_PRELUDE_HLSLI
