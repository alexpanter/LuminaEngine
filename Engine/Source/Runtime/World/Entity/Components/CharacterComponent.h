#pragma once

#include "Core/Object/ObjectMacros.h"
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include "Physics/Physics.h"
#include "CharacterComponent.generated.h"


namespace Lumina
{
    REFLECT(Component)
    struct RUNTIME_API SCharacterPhysicsComponent
    {
        GENERATED_BODY()
    
        JPH::Ref<JPH::CharacterVirtual> Character;
        
        FUNCTION(Script)
        uint32 GetBodyID() const { return Character->GetInnerBodyID().GetIndexAndSequenceNumber(); }
    
        PROPERTY(Script, Editable, Category = "Collision")
        float HalfHeight = 1.8f;

        PROPERTY(Script, Editable, Category = "Collision")
        float Radius = 1.0f;
    
        PROPERTY(Script, Editable, Category = "Physics")
        float Mass = 70.0f;
        
        PROPERTY(Script, Editable, Category = "Physics")
        float Padding = 0.02f;
        
        PROPERTY(Script, Editable, Category = "Physics")
        float PenetrationRecoverySpeed = 1.0f;
        
        PROPERTY(Script, Editable, Category = "Physics")
        float PredictiveContactDistance = 0.1f;
    
        PROPERTY(Script, Editable, Category = "Physics")
        float MaxStrength = 100.0f;
    
        PROPERTY(Script, Editable, Category = "Physics")
        float MaxSlopeAngle = 45.0f;
    
        PROPERTY(Script, Editable, Category = "Physics")
        float StepHeight = 0.4f;
    };

    REFLECT(Component)
    struct RUNTIME_API SCharacterMovementComponent
    {
        GENERATED_BODY()
    
        PROPERTY(Script, Editable, ClampMin = 0.0f, Category = "Movement")
        float MoveSpeed = 5.0f;

        PROPERTY(Script, Editable, ClampMin = 0.0f, Category = "Movement")
        float Acceleration = 10.0f;

        PROPERTY(Script, Editable, ClampMin = 0.0f, Category = "Movement")
        float Deceleration = 8.0f;

        PROPERTY(Script, Editable, ClampMin = 0.0f, Category = "Movement")
        float AirControl = 0.3f;

        PROPERTY(Script, Editable, ClampMin = 0.0f, Category = "Movement")
        float GroundFriction = 8.0f;
    
        PROPERTY(Script, Editable, ClampMin = 0.0f, Category = "Movement")
        float JumpSpeed = 8.0f;
        
        PROPERTY(Script, Editable, ClampMin = 0.0f, ClampMax = 1000.0f, Category = "Movement")
        float RotationRate = 10.0f;
    
        PROPERTY(Script, Editable, ClampMin = 0, Category = "Movement")
        int MaxJumpCount = 1;
    
        PROPERTY(Script, Editable, Category = "Gravity")
        float Gravity = Physics::GEarthGravity;

        PROPERTY(Script, Visible, Category = "Movement")
        glm::vec3 Velocity;
        
        PROPERTY(Script, Editable, Category = "Rotation")
        bool bUseControllerRotation = false;
        
        PROPERTY(Script, Editable, Category = "Rotation")
        bool bOrientRotationToMovement = false;

        PROPERTY(Script, ReadOnly, Category = "Movement")
        bool bGrounded = false;
        
        PROPERTY(Script, ReadOnly, Category = "Movement")
        int JumpCount = 0;
    };

    
}
