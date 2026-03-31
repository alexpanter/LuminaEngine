#pragma once
#include "Containers/String.h"
#include "Platform/Platform.h"


namespace Lumina
{
    
    namespace Audio
    {
        void Initialize();
        void Shutdown();
    }
    
    class RUNTIME_API IAudioContext
    {
    public:
        
        virtual ~IAudioContext() = default;
        
        NODISCARD virtual void* GetNative() const = 0;
        
        virtual void PlaySoundFromFileAtPosition(FStringView File, glm::vec3 Location) = 0;
        virtual void PlaySoundFromFile(FStringView File) = 0;
        virtual void UpdateListenerPosition(glm::vec3 Location, glm::quat Rotation) = 0;
        
        virtual void StopSounds() = 0;
        virtual void Update() = 0;
    
    };
}
