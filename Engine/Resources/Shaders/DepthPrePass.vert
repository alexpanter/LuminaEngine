#version 460 core
#extension GL_ARB_shader_draw_parameters : enable

#pragma shader_stage(vertex)

#include "Includes/SceneGlobals.glsl"

precise invariant gl_Position;

//******* IMPORTANT *******
// Changes to any calculations to gl_Position here, must be exactly reflected in GeometryPass.vert.

void main()
{
    FInstanceData InstanceData  = GetInstanceData(gl_InstanceIndex);
    uint InstanceFlags          = GetInstanceDataFlags(InstanceData);
    
    FVertexData VertexData;
    if(HasFlag(InstanceFlags, INSTANCE_FLAG_SKINNED))
    {
        VertexData = LoadSkinnedVertex(InstanceData.VertexBufferAddress, InstanceData.IndexBufferAddress, gl_VertexIndex, GetInstanceDataBoneIndex(InstanceData));
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
                        
    gl_Position         = Projection * ViewPos;
}
