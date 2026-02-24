#pragma once
#include "Includes/SceneGlobals.slang"

#define LOCAL_SIZE 128

// ============================================================================
//  Helpers
// ============================================================================

bool SphereAABBIntersection(float3 Center, float Radius, float3 AABBMin, float3 AABBMax)
{
    float3 ClosestPoint   = clamp(Center, AABBMin, AABBMax);
    float  DistanceSquared = dot(ClosestPoint - Center, ClosestPoint - Center);
    return DistanceSquared <= Radius * Radius;
}

bool TestSphereAABB(uint i, FCluster Cluster)
{
    float3 Center  = mul(GetCameraView(), float4(uLightData[0].Lights[i].Position, 1.0)).xyz;
    float  Radius  = uLightData[0].Lights[i].Radius;
    return SphereAABBIntersection(Center, Radius, Cluster.MinPoint.xyz, Cluster.MaxPoint.xyz);
}

// ============================================================================
//  Main
// ============================================================================

[shader("compute")]
[numthreads(LOCAL_SIZE, 1, 1)]
void ComputeMain(uint3 WorkGroupID : SV_GroupID, uint3 LocalID : SV_GroupThreadID)
{
    uint Index      = WorkGroupID.x * LOCAL_SIZE + LocalID.x;
    uint LightCount = uLightData[0].NumLights;

    uClusters[Index].Count = 0;

    for (uint i = 0; i < LightCount; ++i)
    {
        if (HasFlag(uLightData[0].Lights[i].Flags, LIGHT_FLAG_TYPE_DIRECTIONAL))
            continue;

        if (TestSphereAABB(i, uClusters[Index]) && uClusters[Index].Count < LIGHTS_PER_CLUSTER)
        {
            uint Slot = uClusters[Index].Count;
            uClusters[Index].LightIndices[Slot] = i;
            uClusters[Index].Count++;
        }
    }
}