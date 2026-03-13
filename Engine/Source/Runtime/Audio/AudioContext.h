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
        virtual void PlaySoundFromFile(FStringView File) = 0;
    
    };
}
