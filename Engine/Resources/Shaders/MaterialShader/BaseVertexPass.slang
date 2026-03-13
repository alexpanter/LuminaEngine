#include "Includes/SceneGlobals.slang"


struct VSOutput
{
    float4 FragColor                : COLOR;
    float4 NormalVS                 : NORMAL_VS;
    float4 NormalWS                 : NORMAL_WS;
    float4 FragPos                  : FRAG_POS;
    float2 UV                       : TEXCOORD0;

    uint InstanceIndex              : INSTANCE;

    // IMPORTANT: Changes to position calculations must be exactly
    // reflected in DepthPrePass.slang
    precise float4 Position         : SV_Position;
};

// ============================================================================
//  Main
// ============================================================================

[shader("vertex")]
VSOutput VertexMain(uint VertexIndex: SV_VulkanVertexID, uint InstanceIndex: SV_VulkanInstanceID)
{
    FGPUInstance InstanceData = GetInstanceData(InstanceIndex);

    FVertexData VertexData;
    if (InstanceData.HasFlag(EInstanceFlags::Skinned))
    {
        VertexData = LoadSkinnedVertex(InstanceData.VBAddress, InstanceData.IBAddress, VertexIndex, InstanceData.BoneOffset);
    }
    else
    {
        VertexData = LoadStaticVertex(InstanceData.VBAddress, InstanceData.IBAddress, VertexIndex);
    }

    float4x4 ModelMatrix = GetModelMatrix(InstanceIndex);
    float4x4 View        = GetCameraView();
    float4x4 Projection  = GetCameraProjection();

    float4 WorldPos = mul(ModelMatrix, float4(VertexData.Position, 1.0));
    float3x3 NormalMatrixWS = CalculateNormalMatrix(ModelMatrix);
    float3 NormalWS = mul(NormalMatrixWS, VertexData.Normal);
    float3 NormalVS = normalize(mul((float3x3)View, NormalWS));
    
    float4 ViewPos  = mul(View, WorldPos);

    VSOutput Output;
    Output.UV               = VertexData.UV;
    Output.FragPos          = ViewPos;
    Output.NormalVS         = float4(NormalVS, 1.0);
    Output.NormalWS         = float4(NormalWS, 1.0);
    Output.InstanceIndex    = InstanceIndex;
    Output.FragColor        = VertexData.Color;
    Output.Position         = mul(Projection, ViewPos);
    
    return Output;
}