#pragma once
#include "Includes/SceneGlobals.slang"

struct VertexOutput
{
    // IMPORTANT: Changes to position calculations here must be
    // exactly reflected in GeometryPass.slang
    precise float4 Position : SV_Position;
};

[shader("vertex")]
VertexOutput VertexMain(uint VertexIndex: SV_VulkanVertexID, uint InstanceIndex: SV_VulkanInstanceID)
{
    FInstanceData InstanceData = GetInstanceData(InstanceIndex);

    FVertexData VertexData;
    if (InstanceData.HasFlag(EInstanceFlags::Skinned))
    {
        VertexData = LoadSkinnedVertex(
            InstanceData.VBAddress,
            InstanceData.IBAddress,
            VertexIndex,
            InstanceData.BoneOffset);
    }
    else
    {
        VertexData = LoadStaticVertex(
            InstanceData.VBAddress,
            InstanceData.IBAddress,
            VertexIndex);
    }

    float4x4 ModelMatrix = GetModelMatrix(InstanceIndex);
    float4x4 View        = GetCameraView();
    float4x4 Projection  = GetCameraProjection();

    float4 WorldPos = mul(ModelMatrix, float4(VertexData.Position, 1.0));
    float4 ViewPos  = mul(View, WorldPos);

    VertexOutput Output;
    Output.Position = mul(Projection, ViewPos);
    return Output;
}