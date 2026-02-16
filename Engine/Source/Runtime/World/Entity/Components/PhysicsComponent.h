#pragma once

#include "Core/Object/ObjectMacros.h"
#include "Physics/PhysicsTypes.h"
#include "PhysicsComponent.generated.h"

namespace Lumina
{
    REFLECT(Component)
    struct RUNTIME_API SRigidBodyComponent
    {
        GENERATED_BODY()
        
        PROPERTY(Script, ReadOnly, Category = "Physics")
        uint32 BodyID = UINT32_MAX;
        
        PROPERTY(Script, Editable, Category = "Physics")
        float Mass = 1.0f;
        
        PROPERTY(Script, Editable, Category = "Physics")
        bool bAllowSleeping = true;
        
        PROPERTY(Script, Editable, ClampMin = 0.001f, Category = "Physics")
        float MaxLinearVelocity = 500.0f;
        
        PROPERTY(Script, Editable, ClampMin = 0.001f, Category = "Physics")
        float MaxAngularVelocity = 0.25f * LE_PI * 60.0f;
        
        PROPERTY(Script, Editable, ClampMin = 0.001f, ClampMax = 1.0f, Category = "Physics")
        float RestitutionOverride = 0.5f;
        
        PROPERTY(Script, Editable, ClampMin = 0.001f, ClampMax = 1.0f, Category = "Physics")
        float FrictionOverride = 0.3f;

        PROPERTY(Script, Editable, ClampMin = 0.001f, ClampMax = 1.0f, Category = "Physics")
        float LinearDamping = 0.0f;

        PROPERTY(Script, Editable, ClampMin = 0.001f, ClampMax = 1.0f, Category = "Physics")
        float AngularDamping = 0.05f;
        
        PROPERTY(Script, Editable, Category = "Physics")
        EBodyType BodyType = EBodyType::Dynamic;
        
        PROPERTY(Script, Editable, Category = "Physics")
        bool bUseGravity = true;
    };

    REFLECT(Component)
    struct RUNTIME_API SBoxColliderComponent
    {
        GENERATED_BODY()

        PROPERTY(Editable)
        glm::vec3 HalfExtent = glm::vec3(0.5f);

        PROPERTY(Editable)
        glm::vec3 Offset;
    };

    REFLECT(Component)
    struct RUNTIME_API SSphereColliderComponent
    {
        GENERATED_BODY()

        PROPERTY(Editable)
        float Radius = 0.5f;

        PROPERTY(Editable)
        glm::vec3 Offset;
    };
    
}