#pragma once
#include "Includes/SceneGlobals.slang"


struct FPushConstants
{
    uint LightIndex;
};

struct VSOutput
{
    float3  WorldPos    : WORLD_POS;
    float3  LightPos    : LIGHT_POS;
    float4  Position    : SV_Position;
};

[shader("vertex")]
VSOutput VertexMain(uint VertexIndex: SV_VulkanVertexID, uint InstanceIndex: SV_VulkanInstanceID, uint ViewIndex : SV_ViewID, uniform FPushConstants PC)
{
    FInstanceData InstanceData = GetInstanceData(InstanceIndex);
    uint          Flags        = GetInstanceDataFlags(InstanceData);

    FVertexData VertexData;
    if (HasFlag(Flags, INSTANCE_FLAG_SKINNED))
    {
        VertexData = LoadSkinnedVertex(
            InstanceData.VBAddress,
            InstanceData.IBAddress,
            VertexIndex,
            GetInstanceDataBoneIndex(InstanceData));
    }
    else
    {
        VertexData = LoadStaticVertex(
            InstanceData.VBAddress,
            InstanceData.IBAddress,
            VertexIndex);
    }

    float4x4 ModelMatrix = GetModelMatrix(InstanceIndex);
    float4   WorldPos    = mul(ModelMatrix, float4(VertexData.Position, 1.0));

    VSOutput Output;
    Output.WorldPos  = WorldPos.xyz;
    Output.LightPos  = uLightData[0].Lights[PC.LightIndex].Position;
    Output.Position  = mul(uLightData[0].Lights[PC.LightIndex].ViewProjection[ViewIndex], WorldPos);
    return Output;
}