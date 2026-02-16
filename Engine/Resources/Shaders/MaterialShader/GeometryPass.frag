#version 460 core

#pragma shader_stage(fragment)

#include "Includes/SceneGlobals.glsl"

layout(early_fragment_tests) in;
layout(location = 0) in vec4 inColor;
layout(location = 1) in vec4 inNormalVS;
layout(location = 2) in vec4 inNormalWS;
layout(location = 3) in vec4 inFragPos;
layout(location = 4) in vec2 inUV;
layout(location = 5) flat in uint inEntityID;

layout(location = 0) out vec4 GNormal;
layout(location = 1) out vec4 GMaterial;
layout(location = 2) out vec4 GAlbedoSpec;
layout(location = 3) out uint GPicker;

uint EntityID       = inEntityID;
vec3 ViewNormal     = normalize(inNormalVS.xyz);
vec3 WorldNormal    = normalize(inNormalWS.xyz);
vec3 WorldPosition  = inFragPos.xyz;
vec2 UV0            = inUV;
vec4 VertexColor    = inColor;

$MATERIAL_INPUTS

void main()
{
    FMaterialInputs Material = GetMaterialInputs();
    
    vec3 EncodedNormal = ViewNormal * 0.5 + 0.5;
    
    GNormal = vec4(EncodedNormal, 1.0);

    GMaterial.r = Material.AmbientOcclusion;
    GMaterial.g = Material.Roughness;
    GMaterial.b = Material.Metallic;
    GMaterial.a = Material.Specular;

    GAlbedoSpec.rgb = Material.Diffuse.rgb;
    GAlbedoSpec.a   = 1.0;

    GPicker = inEntityID;
}
