#include "pch.h"
#include "JoltUtils.h"
#include <glm/detail/type_quat.hpp>
#include "Jolt/Physics/Collision/ObjectLayer.h"


namespace Lumina::JoltUtils
{
    JPH::ObjectLayer PackToObjectLayer(const FCollisionProfile& Profile)
    {
        return ((uint32)Profile.Mask << 16) | (uint32)Profile.Layer;
    }

    JPH::Vec3 ToJPHVec3(const glm::vec3& Vec)
    {
        return JPH::Vec3(Vec.x, Vec.y, Vec.z);
    }
    
    glm::vec3 FromJPHVec3(const JPH::Vec3& Vec)
    {
        return glm::vec3(Vec.GetX(), Vec.GetY(), Vec.GetZ());
    }
    
    JPH::Vec4 ToJPHVec4(const glm::vec4& Vec)
    {
        return JPH::Vec4(Vec.x, Vec.y, Vec.z, Vec.w);
    }
    
    glm::vec4 FromJPHVec4(const JPH::Vec4& Vec)
    {
        return glm::vec4(Vec.GetX(), Vec.GetY(), Vec.GetZ(), Vec.GetW());
    }
    
    JPH::Quat ToJPHQuat(const glm::quat& Quat)
    {
        return JPH::Quat(Quat.x, Quat.y, Quat.z, Quat.w);
    }
    
    glm::quat FromJPHQuat(const JPH::Quat& Quat)
    {
        return glm::quat(Quat.GetW(), Quat.GetX(), Quat.GetY(), Quat.GetZ());
    }
    
    JPH::Mat44 ToJPHMat44(const glm::mat4& Mat)
    {
        return JPH::Mat44(
            JPH::Vec4(Mat[0][0], Mat[0][1], Mat[0][2], Mat[0][3]),
            JPH::Vec4(Mat[1][0], Mat[1][1], Mat[1][2], Mat[1][3]),
            JPH::Vec4(Mat[2][0], Mat[2][1], Mat[2][2], Mat[2][3]),
            JPH::Vec4(Mat[3][0], Mat[3][1], Mat[3][2], Mat[3][3])
        );
    }
    
    glm::mat4 FromJPHMat44(const JPH::Mat44& Mat)
    {
        glm::mat4 Result;
        for (int col = 0; col < 4; ++col)
        {
            JPH::Vec4 Column = Mat.GetColumn4(col);
            Result[col][0] = Column.GetX();
            Result[col][1] = Column.GetY();
            Result[col][2] = Column.GetZ();
            Result[col][3] = Column.GetW();
        }
        return Result;
    }
    
    JPH::RVec3 ToJPHRVec3(const glm::dvec3& Vec)
    {
        return JPH::RVec3(Vec.x, Vec.y, Vec.z);
    }
    
    glm::dvec3 FromJPHRVec3(const JPH::RVec3& Vec)
    {
        return glm::dvec3(Vec.GetX(), Vec.GetY(), Vec.GetZ());
    }
    
    JPH::Color ToJPHColor(const glm::vec4& Color)
    {
        return JPH::Color(
            static_cast<uint8_t>(Color.r * 255.0f),
            static_cast<uint8_t>(Color.g * 255.0f),
            static_cast<uint8_t>(Color.b * 255.0f),
            static_cast<uint8_t>(Color.a * 255.0f)
        );
    }
    
    glm::vec4 FromJPHColor(const JPH::Color& Color)
    {
        return glm::vec4(
            Color.r / 255.0f,
            Color.g / 255.0f,
            Color.b / 255.0f,
            Color.a / 255.0f
        );
    }
    
    JPH::RMat44 ToJPHRMat44(const glm::mat4& Mat)
    {
        return JPH::RMat44(
            JPH::Vec4(Mat[0][0], Mat[0][1], Mat[0][2], Mat[0][3]),
            JPH::Vec4(Mat[1][0], Mat[1][1], Mat[1][2], Mat[1][3]),
            JPH::Vec4(Mat[2][0], Mat[2][1], Mat[2][2], Mat[2][3]),
            JPH::RVec3(Mat[3][0], Mat[3][1], Mat[3][2])
        );
    }
    
    glm::mat4 FromJPHRMat44(const JPH::RMat44& Mat)
    {
        glm::mat4 Result;
        for (int col = 0; col < 3; ++col)
        {
            JPH::Vec4 Column = Mat.GetColumn4(col);
            Result[col][0] = Column.GetX();
            Result[col][1] = Column.GetY();
            Result[col][2] = Column.GetZ();
            Result[col][3] = Column.GetW();
        }
        JPH::RVec3 Translation = Mat.GetTranslation();
        Result[3][0] = static_cast<float>(Translation.GetX());
        Result[3][1] = static_cast<float>(Translation.GetY());
        Result[3][2] = static_cast<float>(Translation.GetZ());
        Result[3][3] = 1.0f;
        return Result;
    }

    JPH::EMotionType ToJoltMotionType(EBodyType Type)
    {
        switch (Type)
        {
            case EBodyType::Static: return JPH::EMotionType::Static;
            case EBodyType::Dynamic: return JPH::EMotionType::Dynamic;
            case EBodyType::Kinematic: return JPH::EMotionType::Kinematic;
        }
        
        UNREACHABLE();
    }
}
