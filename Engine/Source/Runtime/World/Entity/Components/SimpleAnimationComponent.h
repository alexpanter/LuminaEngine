#pragma once
#include "Core/Object/ObjectMacros.h"
#include "Core/Object/ObjectHandleTyped.h"
#include "SimpleAnimationComponent.generated.h"

namespace Lumina
{
    class CAnimation;
    
    REFLECT(Component)
    struct SSimpleAnimationComponent
    {
        GENERATED_BODY()
        
        PROPERTY(Script, Editable, Category = "Animation")
        TObjectPtr<CAnimation> Animation;
        
        PROPERTY(Script, Editable, Category = "Animation")
        float CurrentTime = 0.0f;
        
        PROPERTY(Script, Editable, Category = "Animation")
        float PlaybackSpeed = 1.0f;
        
        PROPERTY(Script, Editable, Category = "Animation")
        bool bLooping = true;
        
        PROPERTY(Script, Editable, Category = "Animation")
        bool bPlaying = true;
    };
}
