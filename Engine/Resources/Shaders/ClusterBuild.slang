#pragma once
#include "Includes/SceneGlobals.slang"

// ============================================================================
//  Push Constants
// ============================================================================

struct FPushConstants
{
    float4x4 InverseProjection;
    float2   ZNearFar;
    uint2    ScreenSize;
    uint4    GridSize;
};

// ============================================================================
//  Helpers
// ============================================================================

float3 ScreenToView(float2 ScreenCoord, FPushConstants PC)
{
    float2 UV       = ScreenCoord / float2(PC.ScreenSize);
    float4 NDC      = float4(UV * 2.0 - 1.0, 1.0, 1.0);
    float4 ViewCoord = mul(PC.InverseProjection, NDC);
    ViewCoord       /= ViewCoord.w;
    return ViewCoord.xyz;
}

float3 LineIntersectionWithZPlane(float3 StartPoint, float3 EndPoint, float ZDistance)
{
    float3 Direction = EndPoint - StartPoint;
    float3 Normal    = float3(0.0, 0.0, -1.0);
    float  T         = (ZDistance - dot(Normal, StartPoint)) / dot(Normal, Direction);
    return StartPoint + T * Direction;
}

// ============================================================================
//  Main
// ============================================================================

[shader("compute")]
[numthreads(1, 1, 1)]
void ComputeMain(uint3 WorkGroupID : SV_GroupID, uniform FPushConstants PC)
{
    float ZNear = PC.ZNearFar.x;
    float ZFar  = PC.ZNearFar.y;

    uint  TileIndex = WorkGroupID.x + (WorkGroupID.y * PC.GridSize.x) + (WorkGroupID.z * PC.GridSize.x * PC.GridSize.y);

    float2 TileSize   = float2(PC.ScreenSize) / float2(PC.GridSize.xy);
    float2 MinTileSS  = float2(WorkGroupID.xy) * TileSize;
    float2 MaxTileSS  = float2(WorkGroupID.xy + 1) * TileSize;

    float3 MinTile = ScreenToView(MinTileSS, PC);
    float3 MaxTile = ScreenToView(MaxTileSS, PC);

    float PlaneNear = -ZNear * pow(ZFar / ZNear,  float(WorkGroupID.z)      / float(PC.GridSize.z));
    float PlaneFar  = -ZNear * pow(ZFar / ZNear,  float(WorkGroupID.z + 1)  / float(PC.GridSize.z));

    float3 MinPointNear = LineIntersectionWithZPlane(float3(0.0), MinTile, PlaneNear);
    float3 MinPointFar  = LineIntersectionWithZPlane(float3(0.0), MinTile, PlaneFar);
    float3 MaxPointNear = LineIntersectionWithZPlane(float3(0.0), MaxTile, PlaneNear);
    float3 MaxPointFar  = LineIntersectionWithZPlane(float3(0.0), MaxTile, PlaneFar);

    float3 MinBounds = min(min(MinPointNear, MinPointFar), min(MaxPointNear, MaxPointFar));
    float3 MaxBounds = max(max(MinPointNear, MinPointFar), max(MaxPointNear, MaxPointFar));

    uClusters[TileIndex].MinPoint = float4(MinBounds, 0.0);
    uClusters[TileIndex].MaxPoint = float4(MaxBounds, 0.0);
}