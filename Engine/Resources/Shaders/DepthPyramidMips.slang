#pragma once
#include "Includes/Common.slang"

struct FPushConstants
{
    float2 TexelSize;
};

// ============================================================================
//  Bindings
// ============================================================================

[[vk::binding(0)]] Sampler2D          SrcMip;
[[vk::binding(1)]] RWTexture2D<float> DstMip;

// ============================================================================
//  Main
// ============================================================================

[shader("compute")]
[numthreads(32, 32, 1)]
void ComputeMain(uint3 GlobalID : SV_DispatchThreadID, uniform FPushConstants PC)
{
    uint2  Pos   = GlobalID.xy;
    float2 UV    = (float2(Pos) + 0.5) / PC.TexelSize;
    float  Depth = SrcMip.Sample(UV).x;

    DstMip[int2(Pos)] = Depth;
}