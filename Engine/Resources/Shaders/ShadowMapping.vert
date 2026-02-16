#version 460

#extension GL_ARB_shader_viewport_layer_array : require
#extension GL_EXT_multiview : require

#pragma shader_stage(vertex)

#include "Includes/SceneGlobals.glsl"

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outLightPos;

layout(push_constant) uniform PushConstants
{
    uint LightIndex;
};

void main()
{
    FInstanceData InstanceData = GetInstanceData(gl_InstanceIndex);

    FVertexData VertexData;
    if(HasFlag(InstanceData.Flags, INSTANCE_FLAG_SKINNED))
    {
        VertexData = LoadSkinnedVertex(InstanceData.VertexBufferAddress, InstanceData.IndexBufferAddress, gl_VertexIndex, InstanceData.BoneOffset);
    }
    else
    {
        VertexData = LoadStaticVertex(InstanceData.VertexBufferAddress, InstanceData.IndexBufferAddress, gl_VertexIndex);
    }

    mat4 ModelMatrix = GetModelMatrix(gl_InstanceIndex);

    vec4 vWorldPos = ModelMatrix * vec4(VertexData.Position, 1.0);

    outWorldPos = vWorldPos.xyz;

    outLightPos = LightData.Lights[LightIndex].Position;

    gl_Position = LightData.Lights[LightIndex].ViewProjection[gl_ViewIndex] * vWorldPos;
}