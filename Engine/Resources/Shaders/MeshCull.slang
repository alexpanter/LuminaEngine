#pragma once
#include "Includes/SceneGlobals.slang"

#define LOCAL_SIZE 256

// ============================================================================
//  Helpers
// ============================================================================

bool IsInFrustum(uint Index)
{
    float4 SphereBounds = uInstanceData[Index].SphereBounds;
    float3 Center       = SphereBounds.xyz;
    float  Radius       = SphereBounds.w;

    float d0 = dot(uSceneData.CullData.Frustum.Planes[0].xyz, Center) + uSceneData.CullData.Frustum.Planes[0].w;
    float d1 = dot(uSceneData.CullData.Frustum.Planes[1].xyz, Center) + uSceneData.CullData.Frustum.Planes[1].w;
    float d2 = dot(uSceneData.CullData.Frustum.Planes[2].xyz, Center) + uSceneData.CullData.Frustum.Planes[2].w;
    float d3 = dot(uSceneData.CullData.Frustum.Planes[3].xyz, Center) + uSceneData.CullData.Frustum.Planes[3].w;
    float d4 = dot(uSceneData.CullData.Frustum.Planes[4].xyz, Center) + uSceneData.CullData.Frustum.Planes[4].w;
    float d5 = dot(uSceneData.CullData.Frustum.Planes[5].xyz, Center) + uSceneData.CullData.Frustum.Planes[5].w;

    float Inside = step(-Radius, d0) *
                   step(-Radius, d1) *
                   step(-Radius, d2) *
                   step(-Radius, d3) *
                   step(-Radius, d4) *
                   step(-Radius, d5);

    return Inside > 0.5;
}

// 2D Polyhedral Bounds of a Clipped, Perspective-Projected 3D Sphere.
// Michael Mara, Morgan McGuire. 2013
bool ProjectSphere(float3 C, float R, float ZNear, float P00, float P11, out float4 AABB)
{
    if (C.z < R + ZNear)
    {
        AABB = float4(0.0, 0.0, 1.0, 1.0);
        return false;
    }

    float2 CX   = -C.xz;
    float2 VX = float2(sqrt(dot(CX, CX) - R * R), R);
    float2 MinX = mul(CX, float2x2(VX.x, VX.y, -VX.y, VX.x));
    float2 MaxX = mul(CX, float2x2(VX.x, -VX.y,  VX.y, VX.x));

    float2 CY   = -C.yz;
    float2 VY = float2(sqrt(dot(CY, CY) - R * R), R);
    float2 MinY = mul(CY, float2x2(VY.x,  VY.y, -VY.y, VY.x));
    float2 MaxY = mul(float2x2(VY.x, -VY.y,  VY.y, VY.x), CY);

    AABB = float4(
        MinX.x / MinX.y * P00,
        MinY.x / MinY.y * P11,
        MaxX.x / MaxX.y * P00,
        MaxY.x / MaxY.y * P11
    );

    // Clip space -> UV space
    AABB = AABB.xwzy * float4(0.5f, -0.5f, 0.5f, -0.5f) + float4(0.5f, 0.5f, 0.5f, 0.5f);

    return true;
}

// ============================================================================
//  Main
// ============================================================================

[shader("compute")]
[numthreads(LOCAL_SIZE, 1, 1)]
void ComputeMain(uint3 GlobalID : SV_DispatchThreadID)
{
    uint GID          = GlobalID.x;
    uint NumInstances = uSceneData.CullData.InstanceNum;

    if (GID >= NumInstances)
        return;

    float ZNear = uSceneData.CullData.zNear;
    float ZFar  = uSceneData.CullData.zFar;

    bool bVisible = true;
    if (uSceneData.CullData.bFrustumCull != 0)
    {
        bVisible = IsInFrustum(GID);

    }

    bool bSkipCull = uInstanceData[GID].HasFlag(EInstanceFlags::NoCull);

    if (!bSkipCull && bVisible && uSceneData.CullData.bOcclusionCull != 0)
    {
        float4 SphereBounds = uInstanceData[GID].SphereBounds;
        float3 Center       = mul(uSceneData.CullData.View, float4(SphereBounds.xyz, 1.0)).xyz;
        float  Radius       = SphereBounds.w;

        float Dist  = length(Center);
        Radius     += Dist * 0.05f;

        Center.y   *= -1.0;

        float4 AABB;
        if (ProjectSphere(Center, Radius, ZNear,
                          uSceneData.CullData.P00,
                          uSceneData.CullData.P11,
                          AABB))
        {
            float Width  = (AABB.z - AABB.x) * uSceneData.CullData.PyramidWidth;
            float Height = (AABB.w - AABB.y) * uSceneData.CullData.PyramidHeight;
            float Level  = floor(log2(max(Width, Height)));

            float2 UV  = float2(AABB.x, AABB.y);
            float2 UV2 = float2(AABB.z, AABB.w);

            float Depth       = uDepthPyramid.SampleLevel((UV + UV2) * 0.5, Level).x;
            float DepthSphere = ZNear / (Center.z - Radius);

            bVisible = bVisible && DepthSphere >= Depth;
        }
    }

    if (bVisible)
    {
        uint BatchID            = uInstanceData[GID].DrawID;
        uint InstanceCount      = 0;
        InterlockedAdd(uIndirectDrawData[BatchID].InstanceCount, 1, InstanceCount);
        uint InstanceIndex      = uIndirectDrawData[BatchID].FirstInstance + InstanceCount;

        uMappingData[InstanceIndex] = GID;
    }
}