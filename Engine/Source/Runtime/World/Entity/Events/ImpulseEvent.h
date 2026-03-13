#pragma once

#include "Core/Object/ObjectMacros.h"
#include "ImpulseEvent.generated.h"

namespace Lumina
{
    REFLECT(Event)
    struct SImpulseEvent
    {
        GENERATED_BODY()
        
        PROPERTY(Script)
        uint32 BodyID;
        
        PROPERTY(Script)
        glm::vec3 Impulse;
    };
    
    REFLECT(Event)
    struct SForceEvent
    {
        GENERATED_BODY()
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 Force;
    };

    REFLECT(Event)
    struct STorqueEvent
    {
        GENERATED_BODY()
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 Torque;
    };

    REFLECT(Event)
    struct SAngularImpulseEvent
    {
        GENERATED_BODY()
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 AngularImpulse;
    };

    REFLECT(Event)
    struct SSetVelocityEvent
    {
        GENERATED_BODY()
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 Velocity;
    };

    REFLECT(Event)
    struct SSetAngularVelocityEvent
    {
        GENERATED_BODY()
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 AngularVelocity;
    };

    REFLECT(Event)
    struct SAddImpulseAtPositionEvent
    {
        GENERATED_BODY()
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 Impulse;
    
        PROPERTY(Script)
        glm::vec3 Position;
    };

    REFLECT(Event)
    struct SAddForceAtPositionEvent
    {
        GENERATED_BODY()
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        glm::vec3 Force;
    
        PROPERTY(Script)
        glm::vec3 Position;
    };

    REFLECT(Event)
    struct SSetGravityFactorEvent
    {
        GENERATED_BODY()
    
        PROPERTY(Script)
        uint32 BodyID;
    
        PROPERTY(Script)
        float GravityFactor;
    };
}
