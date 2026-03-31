#pragma once
#include "Audio/AudioContext.h"
#include "Containers/Array.h"
#include "MiniAudio/miniaudio.h"

namespace Lumina
{
    class FMiniaudioContext : public IAudioContext
    {
    public:
        
        struct FPlayingSound
        {
            TVector<uint8> Bytes;
            ma_decoder Decoder;
            ma_sound Sound;
        };
        
        FMiniaudioContext();
        ~FMiniaudioContext() override;
      
        void* GetNative() const override { return (void*)&Engine; }
        
        void PlaySoundFromFileAtPosition(FStringView File, glm::vec3 Location) override;
        void PlaySoundFromFile(FStringView File) override;
        void UpdateListenerPosition(glm::vec3 Location, glm::quat Rotation) override;
        void Update() override;
        
        void StopSounds() override;
        
    private:
        
        TList<FPlayingSound> PlayingSounds;
        ma_engine Engine;
    };
}
