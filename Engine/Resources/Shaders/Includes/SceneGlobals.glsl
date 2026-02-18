//////////////////////////////////////////////////////////

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference_uvec2 : require
#extension GL_EXT_nonuniform_qualifier : require

#include "Common.glsl"


//////////////////////////////////////////////////////////
// BUFFER REFERENCES
//////////////////////////////////////////////////////////

layout(std430, buffer_reference, buffer_reference_align = 4) readonly buffer IndexBuffer
{
    uint Indices[];
};

layout(std430, buffer_reference, buffer_reference_align = 16) readonly buffer VertexBuffer
{
    FVertex Vertices[];
};

layout(std430, buffer_reference, buffer_reference_align = 16) readonly buffer SkinnedVertexBuffer
{
    FSkinnedVertex Vertices[];
};

//////////////////////////////////////////////////////////
// BINDING SET 0
//////////////////////////////////////////////////////////

layout(set = 0, binding = 0) restrict uniform SceneGlobals
{
    FSceneGlobalData uSceneData;
};

layout(set = 0, binding = 1) restrict readonly buffer SceneLightData
{
    FLightData LightData;
};

layout(set = 0, binding = 2) restrict buffer BufferInstanceData
{
    FInstanceData Instances[];
} InstanceData;

layout(set = 0, binding = 3) restrict buffer InstanceMappingData
{
    uint Mapping[];
} MappingData;

layout(set = 0, binding = 4) restrict buffer FIndirectDrawBuffer
{
    FDrawIndirectArguments Args[];
} IndirectDrawData;

layout(set = 0, binding = 5) restrict readonly buffer FBoneData
{
    mat4 BoneMatrices[];
} BoneData;

layout(set = 0, binding = 6) restrict buffer ClusterSSBO
{
    FCluster Clusters[];
} Clusters;

layout(set = 0, binding = 7) restrict buffer MaterialUniforms
{
    FMaterialUniforms Uniforms[];
} uMaterialUniforms;

layout(set = 0, binding = 8) restrict buffer BillboardInstances
{
    FBillboardInstance Billboards[];
};

layout(set = 0, binding = 9)        uniform sampler2DArray uShadowCascade;
layout(set = 0, binding = 10)       uniform sampler2DArray uShadowAtlas;
layout(set = 0, binding = 11)       uniform usampler2D uSelectionTexture;
layout(set = 0, binding = 12)       uniform sampler2D uDepthPyramid;
layout(set = 0, binding = 13)       uniform sampler2D uHDRSceneColor;


//////////////////////////////////////////////////////////
// BINDING SET 1
//////////////////////////////////////////////////////////

layout(set = 1, binding = 0) uniform sampler2D uGlobalTextures[];

//////////////////////////////////////////////////////////

FVertexData LoadStaticVertex(uvec2 VertexAddress, uvec2 IndexAddress, uint VertexID)
{
    IndexBuffer IBuf    = IndexBuffer(IndexAddress);
    uint Index          = IBuf.Indices[VertexID];
    
    VertexBuffer VBuf   = VertexBuffer(VertexAddress);
    FVertex V           = VBuf.Vertices[Index];

    FVertexData Data;
    Data.Position   = V.Position;
    Data.Normal     = UnpackNormal(V.Normal);
    Data.UV         = UnpackUV(V.UV);
    Data.Color      = UnpackColor(V.Color);
    
    return Data;
}

FVertexData LoadSkinnedVertex(uvec2 VertexAddress, uvec2 IndexAddress, uint VertexID, uint BoneOffset)
{
    IndexBuffer IBuf = IndexBuffer(IndexAddress);
    uint Index = IBuf.Indices[VertexID];
    
    SkinnedVertexBuffer VBuf = SkinnedVertexBuffer(VertexAddress);
    FSkinnedVertex V = VBuf.Vertices[Index];

    vec3 LocalPos = V.Position;
    vec3 LocalNormal = UnpackNormal(V.Normal);

    uvec4 JointIndices = UnpackUIntToUVec4(V.JointIndices);
    vec4 Weights = unpackUnorm4x8(V.JointWeights);

    mat4 SkinMatrix =
    BoneData.BoneMatrices[BoneOffset + JointIndices.x] * Weights.x +
    BoneData.BoneMatrices[BoneOffset + JointIndices.y] * Weights.y +
    BoneData.BoneMatrices[BoneOffset + JointIndices.z] * Weights.z +
    BoneData.BoneMatrices[BoneOffset + JointIndices.w] * Weights.w;

    FVertexData Data;
    Data.Position       = (SkinMatrix * vec4(LocalPos, 1.0)).xyz;
    Data.Normal         = normalize(mat3(SkinMatrix) * LocalNormal);
    Data.UV             = UnpackUV(V.UV);
    Data.Color          = UnpackColor(V.Color);
    
    return Data;
}

float GetMaterialScalar(uint MatIndex, uint Index)
{
    return uMaterialUniforms.Uniforms[MatIndex].Scalars[Index];
}

vec4 GetMaterialVec4(uint MatIndex, uint Index)
{
    return uMaterialUniforms.Uniforms[MatIndex].Vectors[Index];
}

uint GetMaterialTexture(uint MatIndex, uint Index)
{
    return uMaterialUniforms.Uniforms[MatIndex].Textures[Index];
}

uint DrawIDToInstanceID(uint ID)
{
    return MappingData.Mapping[ID];
}

FInstanceData GetInstanceData(uint Index)
{
    return InstanceData.Instances[DrawIDToInstanceID(Index)];
}

vec3 GetSunDirection()
{
    return LightData.SunDirection.xyz;
}

vec3 GetAmbientLightColor()
{
    return LightData.AmbientLight.xyz;
}

float GetAmbientLightIntensity()
{
    return LightData.AmbientLight.w;
}

float GetTime()
{
    return uSceneData.Time;
}

float GetDeltaTime()
{
    return uSceneData.DeltaTime;
}

float GetNearPlane()
{
    return uSceneData.NearPlane;
}

float GetFarPlane()
{
    return uSceneData.FarPlane;
}

vec3 GetCameraPosition()
{
    return uSceneData.CameraView.CameraPosition.xyx;
}

vec3 GetCameraUp()
{
    return uSceneData.CameraView.CameraUp.xyx;
}

vec3 GetCameraRight()
{
    return uSceneData.CameraView.CameraRight.xyx;
}

vec3 GetCameraForward()
{
    return uSceneData.CameraView.CameraForward.xyx;
}

mat4 GetCameraView()
{
    return uSceneData.CameraView.CameraView;
}

mat4 GetInverseCameraView()
{
    return uSceneData.CameraView.InverseCameraView;
}

mat4 GetCameraProjection()
{
    return uSceneData.CameraView.CameraProjection;
}

mat4 GetInverseCameraProjection()
{
    return uSceneData.CameraView.InverseCameraProjection;
}

mat4 GetModelMatrix(uint Index)
{
    return InstanceData.Instances[DrawIDToInstanceID(Index)].ModelMatrix;
}

vec3 GetModelLocation(uint Index)
{
    return vec3(GetModelMatrix(Index)[3].xyz);
}

uint GetEntityID(uint Index)
{
    return InstanceData.Instances[DrawIDToInstanceID(Index)].EntityID;
}

vec3 WorldToView(vec3 WorldPos)
{
    return (GetCameraView() * vec4(WorldPos, 1.0)).xyz;
}

vec3 NormalWorldToView(vec3 Normal)
{
    return mat3(GetCameraView()) * Normal;
}

vec4 ViewToClip(vec3 ViewPos)
{
    return GetCameraProjection() * vec4(ViewPos, 1.0);
}

vec4 WorldToClip(vec3 WorldPos)
{
    return GetCameraProjection() * GetCameraView() * vec4(WorldPos, 1.0);
}

float SineWave(float speed, float amplitude)
{
    return amplitude * sin(float(GetTime()) * speed);
}

FLight GetLightAt(uint Index)
{
    return LightData.Lights[Index];
}

uint GetNumLights()
{
    return LightData.NumLights;
}

vec3 ReconstructWorldPos(vec2 uv, float depth, mat4 invProj, mat4 invView)
{
    vec4 clip = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 view = invProj * clip;
    view /= view.w;
    vec4 world = invView * view;
    return world.xyz;
}

vec3 ReconstructViewPos(vec2 uv, float depth, mat4 invProj)
{
    vec4 clip = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 view = invProj * clip;
    return view.xyz / view.w;
}