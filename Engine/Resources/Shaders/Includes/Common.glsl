
//// ---------------------------------------------------------------------------------
////  Constants


const float PHI     = 1.618033988749895; // Golden ratio
const float PI      = 3.1415926535897932384626433832795;
const float TWO_PI  = 6.28318530718;
const float INV_PI  = 0.31830988618;


#define LIGHT_INDEX_MASK 0x1FFFu
#define LIGHTS_PER_UINT 2
#define LIGHTS_PER_CLUSTER 100
#define SHADOW_SAMPLE_COUNT 14

#define INDEX_NONE -1

#define COL_R_SHIFT 0
#define COL_G_SHIFT 8
#define COL_B_SHIFT 16
#define COL_A_SHIFT 24
#define COL_A_MASK 0xFF000000

#define MAX_LIGHTS 1728
#define MAX_SHADOWS 100
#define NUM_CASCADES 3

#define MAX_SCALARS 24
#define MAX_VECTORS 24
#define MAX_TEXTURES 24

//////////////////////////////////////////////////////////

const int SSAO_KERNEL_SIZE        = 32;

//////////////////////////////////////////////////////////


#define BIT(X) (1u << (X))


const uint LIGHT_FLAG_TYPE_DIRECTIONAL = BIT(0);
const uint LIGHT_FLAG_TYPE_POINT       = BIT(1);
const uint LIGHT_FLAG_TYPE_SPOT        = BIT(2);
const uint LIGHT_FLAG_CASTSHADOW       = BIT(3);

//////////////////////////////////////////////////////////

#define INSTANCE_FLAG_BILLBOARD         BIT(0)
#define INSTANCE_FLAG_SKINNED           BIT(1)
#define INSTANCE_FLAG_SELECTED          BIT(2)
#define INSTANCE_FLAG_CAST_SHADOW       BIT(3)
#define INSTANCE_FLAG_RECEIVE_SHADOW    BIT(4)

//////////////////////////////////////////////////////////


// Unpack 10:10:10:2 normal
vec3 UnpackNormal(uint packed)
{
    vec3 normal;
    normal.x = float(int(packed << 22) >> 22) / 511.0;
    normal.y = float(int(packed << 12) >> 22) / 511.0;
    normal.z = float(int(packed << 2) >> 22) / 511.0;
    return normalize(normal);
}

vec2 UnpackUV(uint packed) 
{
    return unpackHalf2x16(packed);
}

mat3 CalculateNormalMatrix(mat4 ModelMatrix)
{
    vec3 C0 = ModelMatrix[0].xyz;
    vec3 C1 = ModelMatrix[1].xyz;
    vec3 C2 = ModelMatrix[2].xyz;
    
    vec3 R0 = cross(C1, C2);
    vec3 R1 = cross(C2, C0);
    vec3 R2 = cross(C0, C1);
    
    float InvDet = 1.0 / dot(C0, R0);
    
    return mat3(R0 * InvDet, R1 * InvDet, R2 * InvDet);
}

vec2 VogelDiskSample(int SampleIndex, int SamplesCount, float Angle)
{
    float GoldenAngle   = 2.0 * PI * (1.0 - 1.0 / PHI);
    float r             = sqrt((float(SampleIndex) + 0.5) / float(SamplesCount));
    float theta         = float(SampleIndex) * GoldenAngle + Angle;
    
    return vec2(r * cos(theta), r * sin(theta));
}

vec3 ScreenSpaceDither(vec2 vScreenPos, float Time)
{
    // Iestyn's RGB dither (7 asm instructions) from Portal 2 X360, slightly modified.
    vec3 vDither = vec3(dot(vec2(171.0, 231.0), vScreenPos.xy + Time));
    vDither.rgb = fract(vDither.rgb / vec3(103.0, 71.0, 97.0)) - vec3(0.5, 0.5, 0.5);
    return (vDither.rgb / 255.0) * 0.375;
}

uvec4 UnpackUIntToUVec4(uint packed)
{
    return uvec4(
    (packed      ) & 0xFFu,
    (packed >> 8 ) & 0xFFu,
    (packed >> 16) & 0xFFu,
    (packed >> 24) & 0xFFu
    );
}

struct FVertex
{
    vec3    Position;
    uint    Normal;
    uint    UV;
    uint    Color;
};

struct FSkinnedVertex
{
    vec3  Position;
    uint  Normal;
    uint  UV;
    uint  Color;
    
    uint JointIndices;
    uint JointWeights;
};

// Data actually used in shader after access.
struct FVertexData
{
    vec3 Position;
    vec3 Normal;
    vec2 UV;
    vec4 Color;
};

struct FDrawIndirectArguments
{
    uint VertexCount;
    uint InstanceCount;
    uint VertexOffset;
    uint FirstInstance;
};

struct FDrawIndexedIndirectArguments
{
    uint IndexCount;
    uint InstanceCount;
    uint FirstIndex;
    int  VertexOffset;
    uint FirstInstance;
};

struct FCameraView
{
    vec4 CameraPosition;    // Camera Position
    vec4 CameraUp;
    vec4 CameraRight;
    vec4 CameraForward;
    mat4 CameraView;        // View matrix
    mat4 InverseCameraView;
    mat4 CameraProjection;  // Projection matrix
    mat4 InverseCameraProjection; // Inverse Camera Projection.
};

struct FMaterialUniforms
{
    vec4    Vectors[MAX_VECTORS];
    float   Scalars[MAX_SCALARS];
    uint    Textures[MAX_TEXTURES];
};

struct FMaterialInputs
{
    vec3    Diffuse;
    float   Metallic;
    float   Roughness;
    float   Specular;
    vec3    Emissive;
    float   AmbientOcclusion;
    vec3    Normal;
    float   Opacity;
    vec3    WorldPositionOffset;
};

struct FEntityAABB
{
    uint  Entity;
    float MinX;
    float MinY;
    float MaxX;
    float MaxY;
};

struct FSSAOSettings
{
    float Radius;
    float Intensity;
    float Power;

    vec4 Samples[SSAO_KERNEL_SIZE];
};

struct FInstanceData
{
    mat3x4      ModelMatrix;
    vec4        SphereBounds;

    uvec2       VertexBufferAddress;
    uvec2       IndexBufferAddress;
    
    uint        EntityID;
    uint        DrawIDAndFlags;
    uint        BoneOffsetAndMaterialIndex;
};

struct FLightShadow
{
    vec2 AtlasUVOffset;
    vec2 AtlasUVScale;
    int ShadowMapIndex; // -1 means no shadow.
    int ShadowMapLayer;
    int LightIndex;
    int Pad;
};

struct FBillboardInstance
{
    vec3    Position;
    float   Size;
    uint    TextureIndex;
    uint    EntityID;
};

struct FLight
{
    vec3 Position;
    uint Color;

    float Intensity;
    float Padding[3];
    
    vec3 Direction;
    float Radius;
    
    mat4 ViewProjection[6];
    
    vec2 Angles;
    uint Flags;
    float Falloff;

    FLightShadow Shadow[6];
};

struct FLightData
{
    uint    NumLights;
    uint    Padding[3];

    vec3    SunDirection;
    uint    bHasSun;

    vec4    CascadeSplits;

    vec4    AmbientLight;

    FLight  Lights[MAX_LIGHTS];
};

struct FCluster
{
    vec4 MinPoint;
    vec4 MaxPoint;
    uint LightIndices[LIGHTS_PER_CLUSTER];
    uint Count;
};

struct FFrustum
{
    vec4 Planes[6];
};

struct FCullData
{
    FFrustum Frustum;
    mat4 View;

    float P00;              // projection[0][0]
    float P11;              // projection[1][1]
    float zNear;
    float zFar;

    uint bFrustumCull;
    uint bOcclusionCull;
    uint InstanceNum;
    uint Padding0;
    
    float PyramidWidth;
    float PyramidHeight;
};

struct FSceneGlobalData
{
    FCameraView CameraView;
    uvec4 ScreenSize;
    uvec4 GridSize;

    float Time;
    float DeltaTime;
    float NearPlane;
    float FarPlane;

    FSSAOSettings   SSAOSettings;
    FCullData       CullData;
};

vec4 UnpackColor(uint ColorMask)
{
    return vec4
    (
        float((ColorMask >> COL_R_SHIFT) & uint(0xFF)) / 255.0,
        float((ColorMask >> COL_G_SHIFT) & uint(0xFF)) / 255.0,
        float((ColorMask >> COL_B_SHIFT) & uint(0xFF)) / 255.0,
        float((ColorMask >> COL_A_SHIFT) & uint(0xFF)) / 255.0
    );
}

vec4 GetLightColor(FLight Light)
{
    return UnpackColor(Light.Color);
}

// Check if *any* of the bits in 'flag' are set in 'value'
bool HasFlag(uint value, uint flag)
{
    return (value & flag) != 0u;
}

// Check if *all* bits in 'flag' are set in 'value'
bool HasAllFlags(uint value, uint flag)
{
    return (value & flag) == flag;
}

// Set bits in 'flag' on 'value'
uint SetFlag(uint value, uint flag)
{
    return value | flag;
}

// Clear bits in 'flag' from 'value'
uint ClearFlag(uint value, uint flag)
{
    return value & (~flag);
}

// Toggle bits in 'flag' on 'value'
uint ToggleFlag(uint value, uint flag)
{
    return value ^ flag;
}


vec3 ReconstructWorldPosition(vec2 uv, float depth, mat4 invProjection, mat4 invView) 
{
    float z = depth * 2.0 - 1.0;
    
    vec4 ClipSpacePos = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 ViewSpacePos = invProjection * ClipSpacePos;
    
    ViewSpacePos /= ViewSpacePos.w;
    
    vec4 WorldSpacePosition = invView * ViewSpacePos;
    
    return WorldSpacePosition.xyz;
}

vec3 ReconstructViewPosition(vec2 uv, float depth, mat4 invProjection)
{
    // Convert UV and reversed depth to clip space
    vec4 clipSpace = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);

    // Transform to view space
    vec4 viewSpace = invProjection * clipSpace;

    // Perspective divide
    return viewSpace.xyz / viewSpace.w;
}

float LinearizeDepth(float depth, float near, float far) 
{
    return (near * far) / (far - depth * (far - near)) / far;
}

float LinearizeDepthReverseZ(float depth, float near, float far)
{
    return near * far / (far + depth * (near - far));
}