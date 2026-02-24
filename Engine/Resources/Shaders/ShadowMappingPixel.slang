#pragma once
#include "Includes/SceneGlobals.slang"

// ============================================================================
//  Push Constants
// ============================================================================

struct FPushConstants
{
    uint LightIndex;
};

// ============================================================================
//  Input
// ============================================================================

struct PSInput
{
    float3 WorldPos : WORLD_POS;
    float3 LightPos : LIGHT_POS;
};

// ============================================================================
//  Main
// ============================================================================

[shader("fragment")]
float FragmentMain(PSInput Input, FPushConstants PC) : SV_Depth
{
    FLight Light    = GetLightAt(PC.LightIndex);
    float  Distance = length(Input.WorldPos - Input.LightPos);
    return Distance / Light.Radius;
}