#version 460 core

#pragma shader_stage(vertex)

#include "Includes/SceneGlobals.glsl"

layout(location = 0) out vec2 vUV;
layout(location = 1) flat out uint TextureIndex;
layout(location = 2) flat out uint EntityID;

const vec2 Positions[6] = vec2[]
(
    vec2(-1.0, -1.0),  // Triangle 1
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0),
    
    vec2(-1.0, -1.0),  // Triangle 2
    vec2( 1.0,  1.0),
    vec2(-1.0,  1.0)
);

const vec2 UVs[6] = vec2[]
(
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),
    
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0)
);

void main()
{
    vec2 QuadPos = Positions[gl_VertexIndex];
    vUV = UVs[gl_VertexIndex];
    
    vec3 BillboardCenter    = Billboards[gl_InstanceIndex].Position;
    float BillboardSize     = Billboards[gl_InstanceIndex].Size;
    TextureIndex            = Billboards[gl_InstanceIndex].TextureIndex;
    EntityID                = Billboards[gl_InstanceIndex].EntityID;
    

    mat4 ViewMatrix         = GetCameraView();

    vec3 CameraRight_WS     =  vec3(ViewMatrix[0][0], ViewMatrix[1][0], ViewMatrix[2][0]);
    vec3 CameraUp_WS        = -vec3(ViewMatrix[0][1], ViewMatrix[1][1], ViewMatrix[2][1]);
    
    vec3 VertexPosition_WS  = BillboardCenter 
    + CameraRight_WS    * QuadPos.x     * BillboardSize
    + CameraUp_WS       * QuadPos.y     * BillboardSize;

    gl_Position         = GetCameraProjection() * GetCameraView() * vec4(VertexPosition_WS, 1.0);    
}