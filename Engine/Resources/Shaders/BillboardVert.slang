#pragma once
#include "Includes/SceneGlobals.slang"


// ============================================================================
//  Quad geometry
// ============================================================================

static const float2 Positions[6] =
{
    float2(-1.0, -1.0),  // Triangle 1
    float2( 1.0, -1.0),
    float2( 1.0,  1.0),

    float2(-1.0, -1.0),  // Triangle 2
    float2( 1.0,  1.0),
    float2(-1.0,  1.0)
};

static const float2 UVs[6] =
{
    float2(0.0, 0.0),
    float2(1.0, 0.0),
    float2(1.0, 1.0),

    float2(0.0, 0.0),
    float2(1.0, 1.0),
    float2(0.0, 1.0)
};

// ============================================================================
//  IO
// ============================================================================

struct VertexOutput
{
    float2 UV                         : TEXCOORD0;
    nointerpolation uint TextureIndex : TEXTURE_INDEX;
    nointerpolation uint EntityID     : ENTITY_ID;
    float4 Position                   : SV_Position;
};

// ============================================================================
//  Main
// ============================================================================

[shader("vertex")]
VertexOutput VertexMain(
    uint VertexIndex   : SV_VertexID,
    uint InstanceIndex : SV_InstanceID)
{
    float2 QuadPos = Positions[VertexIndex];

    FBillboardInstance Billboard = uBillboards[InstanceIndex];

    float4x4 ViewMatrix = GetCameraView();

    float3 CameraRight_WS =  float3(ViewMatrix[0][0], ViewMatrix[1][0], ViewMatrix[2][0]);
    float3 CameraUp_WS    = -float3(ViewMatrix[0][1], ViewMatrix[1][1], ViewMatrix[2][1]);

    float3 WorldPos = Billboard.Position
                    + CameraRight_WS * QuadPos.x * Billboard.Size
                    + CameraUp_WS    * QuadPos.y * Billboard.Size;

    VertexOutput Output;
    Output.UV           = UVs[VertexIndex];
    Output.TextureIndex = Billboard.TextureIndex;
    Output.EntityID     = Billboard.EntityID;
    Output.Position     = mul(GetCameraProjection(), mul(GetCameraView(), float4(WorldPos, 1.0)));
    return Output;
}