#pragma once
#include "Includes/SceneGlobals.slang"

// ============================================================================
//  IO
// ============================================================================

struct FragmentInput
{
    float2 UV                       : TEXCOORD0;
    nointerpolation uint TextureIndex : TEXTURE_INDEX;
    nointerpolation uint EntityID     : ENTITY_ID;
};

struct FragmentOutput
{
    float4 Color  : SV_Target0;
    uint   Picker : SV_Target1;
};

// ============================================================================
//  Main
// ============================================================================

[shader("fragment")]
FragmentOutput FragmentMain(FragmentInput Input)
{
    float4 Texture = uGlobalTextures[Input.TextureIndex].Sample(int2(Input.UV));

    if (Texture.a <= 0.0)
        discard;

    FragmentOutput Output;
    Output.Color  = Texture;
    Output.Picker = Input.EntityID;
    return Output;
}