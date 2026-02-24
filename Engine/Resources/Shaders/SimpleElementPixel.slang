#pragma once
#include "Includes/SceneGlobals.slang"

[shader("fragment")]
float4 FragmentMain(float3 Color: COLOR) : SV_Target0
{
    return float4(Color, 1.0);
}