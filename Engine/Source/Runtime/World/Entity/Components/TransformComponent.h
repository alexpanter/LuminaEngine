#pragma once

#include <glm/glm.hpp>
#include "Core/Math/Transform.h"
#include "TransformComponent.generated.h"

namespace Lumina
{
    REFLECT(Component, HideInComponentList)
    struct RUNTIME_API STransformComponent
    {
        GENERATED_BODY()
        
        STransformComponent() = default;
        STransformComponent(const FTransform& InTransform)
            : Transform(InTransform)
        {}
        
        STransformComponent(const glm::vec3& InPosition)
            :Transform(InPosition)
        {}

        STransformComponent(const glm::mat4& InMatrix)
            : Transform(InMatrix)
        {}

        FUNCTION(Script)
        FORCEINLINE FTransform& GetTransform() { return Transform; }

        FUNCTION(Script)
        FORCEINLINE void SetTransform(const FTransform& InTransform) { Transform = InTransform; }

        FUNCTION(Script)
        FORCEINLINE glm::vec3 GetLocation() const    { return Transform.Location; }
        
        FUNCTION(Script)
        FORCEINLINE glm::vec3 GetPosition() const    { return Transform.Location; }

        FUNCTION(Script)
        FORCEINLINE glm::quat GetRotation() const    { return Transform.Rotation; }

        FUNCTION(Script)
        FORCEINLINE glm::vec3 GetScale()    const    { return Transform.Scale; }
        
        FUNCTION(Script)
        FORCEINLINE float MaxScale()        const    { return glm::max(Transform.Scale.x, glm::max(Transform.Scale.y, Transform.Scale.z)); }
        
        FORCEINLINE glm::mat4 GetMatrix()   const    { return CachedMatrix; }

        FUNCTION(Script)
        FORCEINLINE glm::vec3& Translate(const glm::vec3& Translation)
        {
            Transform.Translate(Translation);
            return Transform.Location;
        }

        FUNCTION(Script)
        FORCEINLINE glm::vec3 SetLocation(const glm::vec3& InLocation) 
        { 
            Transform.Location = InLocation;
            return Transform.Location;
        }

        FUNCTION(Script)
        FORCEINLINE glm::quat SetRotation(const glm::quat& InRotation)
        { 
            Transform.Rotation = InRotation;
            return Transform.Rotation;
        }

        FUNCTION(Script)
        FORCEINLINE glm::vec3 SetRotationFromEuler(const glm::vec3& EulerRotation)
        {
            Transform.Rotation = glm::quat(glm::radians(EulerRotation));
            return GetRotationAsEuler();
        }

        FUNCTION(Script)
        FORCEINLINE glm::vec3 SetScale(const glm::vec3& InScale) 
        { 
            Transform.Scale = InScale;
            return Transform.Scale;
        }

        FUNCTION(Script)
        FORCEINLINE glm::vec3 GetRotationAsEuler() const 
        {
            return glm::degrees(glm::eulerAngles(Transform.Rotation));
        }

        FUNCTION(Script)
        FORCEINLINE void AddYaw(float Degrees)
        {
            glm::quat YawQuat = glm::angleAxis(glm::radians(Degrees), glm::vec3(0.0f, 1.0f, 0.0f));
            Transform.Rotation = glm::normalize(YawQuat * Transform.Rotation);
        }

        FUNCTION(Script)
        FORCEINLINE void AddPitch(float Degrees)
        {
            glm::quat PitchQuat = glm::angleAxis(glm::radians(Degrees), Transform.GetRight());
            Transform.Rotation = glm::normalize(PitchQuat * Transform.Rotation);
        }

        FUNCTION(Script)
        FORCEINLINE void AddRoll(float Degrees)
        {
            glm::quat RollQuat = glm::angleAxis(glm::radians(Degrees), Transform.GetForward());
            Transform.Rotation = glm::normalize(RollQuat * Transform.Rotation);
        }
        
        FUNCTION(Script)
        FORCEINLINE glm::vec3 GetForward() const
        {
            return Transform.GetForward();
        }

        FUNCTION(Script)
        FORCEINLINE glm::vec3 GetRight() const
        {
            return Transform.GetRight();
        }

        FUNCTION(Script)
        FORCEINLINE glm::vec3 GetUp() const
        {
            return Transform.GetUp();
        }
        
    public:

        /** The local transform of an entity */
        PROPERTY(Editable, Category = "Transform")
        FTransform Transform;

        /** World transform of an entity */
        FTransform WorldTransform = Transform;

        /** The cached matrix always refers to the matrix in world space.*/
        glm::mat4 CachedMatrix = WorldTransform.GetMatrix();
    };
    
}
