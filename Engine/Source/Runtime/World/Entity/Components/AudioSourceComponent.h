#pragma once
#include "Core/Object/ObjectMacros.h"
#include "AudioSourceComponent.generated.h"

namespace Lumina
{
    REFLECT(Component)
    struct RUNTIME_API SAudioSourceComponent
    {
        GENERATED_BODY()
        
        PROPERTY(Script, Editable)
        FString SoundFile;

        PROPERTY(Script, Editable)
        float Volume = 1.0f;

        PROPERTY(Script, Editable)
        float Pitch = 1.0f;

        PROPERTY(Script, Editable)
        float MinDistance = 1.0f;

        PROPERTY(Script, Editable)
        float MaxDistance = 50.0f;

        PROPERTY(Script, Editable)
        bool bLooping = false;

        PROPERTY(Script, Editable)
        bool bPlayOnReady = false;
    };
    
    REFLECT(Component)
    struct RUNTIME_API SAudioListenerComponent
    {
        GENERATED_BODY()
        
        uint8 Foobar;
    };
}
