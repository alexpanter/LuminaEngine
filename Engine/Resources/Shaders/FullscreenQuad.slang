#pragma once

struct VSOutput
{
    float4 Position : SV_Position;
    float2 UV       : TEXCOORD0;
};

[shader("vertex")]
VSOutput VertexMain(uint VertexIndex : SV_VertexID)
{
    VSOutput Output;
    Output.UV       = float2((VertexIndex << 1) & 2, VertexIndex & 2);
    Output.Position = float4(Output.UV * 2.0 - 1.0, 1.0, 1.0);
    return Output;
}