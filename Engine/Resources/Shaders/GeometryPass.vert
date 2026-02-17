#version 460 core

#extension GL_ARB_shader_draw_parameters : enable
#extension GL_EXT_buffer_reference_uvec2 : require

#pragma shader_stage(vertex)

#include "Includes/SceneGlobals.glsl"

// Outputs
layout(location = 0) out vec4 outFragColor;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outNormalWS;
layout(location = 3) out vec4 outFragPos;
layout(location = 4) out vec2 outUV;
layout(location = 5) flat out uint outEntityID;
layout(location = 6) flat out uint outReceiveShadow;
layout(location = 7) flat out uint outSelected;
layout(location = 8) flat out uint outMatIndex;

precise invariant gl_Position;

//******* IMPORTANT *******
// Changes to any calculations to gl_Position here, must be exactly reflected in DepthPrePass.vert.

void main()
{
    FInstanceData InstanceData  = GetInstanceData(gl_InstanceIndex);
    
    FVertexData VertexData;
    if(HasFlag(InstanceData.Flags, INSTANCE_FLAG_SKINNED))
    {
        VertexData = LoadSkinnedVertex(InstanceData.VertexBufferAddress, InstanceData.IndexBufferAddress, gl_VertexIndex, InstanceData.BoneOffset);
    }
    else
    {
        VertexData = LoadStaticVertex(InstanceData.VertexBufferAddress, InstanceData.IndexBufferAddress, gl_VertexIndex);
    }
    
    mat4 ModelMatrix    = GetModelMatrix(gl_InstanceIndex);
    mat4 View           = GetCameraView();
    mat4 Projection     = GetCameraProjection();

    vec4 WorldPos       = ModelMatrix * vec4(VertexData.Position, 1.0);
    vec4 ViewPos        = View * WorldPos;
    
    // World-space
    mat3 NormalMatrixWS = CalculateNormalMatrix(ModelMatrix);
    vec3 NormalWS       = NormalMatrixWS * VertexData.Normal;
    
    // View-space
    vec3 NormalVS       = normalize(mat3(View) * NormalWS);

    // Outputs
    outUV               = vec2(VertexData.UV.x, 1.0 - VertexData.UV.y);
    outFragPos          = ViewPos;
    outNormal           = vec4(NormalVS, 1.0);
    outNormalWS         = vec4(NormalWS, 1.0);
    outFragColor        = VertexData.Color;
    outEntityID         = InstanceData.EntityID;
    outReceiveShadow    = uint(HasFlag(InstanceData.Flags, INSTANCE_FLAG_RECEIVE_SHADOW));
    outSelected         = uint(HasFlag(InstanceData.Flags, INSTANCE_FLAG_SELECTED));
    outMatIndex         = InstanceData.MaterialIndex;
    gl_Position         = Projection * ViewPos;
}
