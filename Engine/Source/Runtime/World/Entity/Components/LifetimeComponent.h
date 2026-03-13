#pragma once
#include "Core/Object/ObjectMacros.h"
#include "LifetimeComponent.generated.h"

namespace Lumina
{
    REFLECT(Component, HideInComponentList)
    struct RUNTIME_API SLifetimeComponent
    {
        GENERATED_BODY()
        
        PROPERTY(Script)
        float Lifetime = 0.0f;
    };
}
