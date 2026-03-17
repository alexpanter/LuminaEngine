#pragma once
#include "Core/Object/ObjectMacros.h"
#include "WorldSettings.generated.h"


namespace Lumina
{
    REFLECT(Component, HideInComponentList)
    struct RUNTIME_API SDefaultWorldSettings
    {
        GENERATED_BODY()
        
        PROPERTY(Editable)
        float WorldKillHeight = -5'000;
        
        PROPERTY(Editable)
        float GravityScale = 1.0f;
        
        PROPERTY(Editable)
        float FixedPhysicsTimestep = 60.0f;
        
    };
}
