#pragma once

#include "Core/Object/ObjectMacros.h"
#include "HealthComponent.generated.h"

namespace Lumina
{
    REFLECT(Component)
    struct RUNTIME_API SHealthComponent
    {
        GENERATED_BODY()
        
        FUNCTION(Script)
        float ApplyDamage(float Damage) { Health -= Damage; return Health; }
        
        FUNCTION(Script)
        void GiveHealth(float NewHealth) { Health = glm::clamp(Health + NewHealth, 0.0f, MaxHealth); }
        
        PROPERTY(Script, Editable, Category = "Health")
        float Health = 100.0f;
        
        PROPERTY(Script, Editable, Category = "Health")
        float MaxHealth = 100.0f;
    };
}