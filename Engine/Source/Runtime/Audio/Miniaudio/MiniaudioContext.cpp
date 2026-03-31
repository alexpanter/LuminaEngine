#include "pch.h"
#include "MiniaudioContext.h"

#include <glm/gtx/quaternion.hpp>
#include "FileSystem/FileSystem.h"
#include "Containers/Array.h"
#include "MiniAudio/miniaudio.h"

namespace Lumina
{
    FMiniaudioContext::FMiniaudioContext()
    {
        ma_engine_config Config = ma_engine_config_init();
        
        ma_result Result = ma_engine_init(&Config, &Engine);
        if (Result != MA_SUCCESS) 
        {
            return;
        }
    }

    FMiniaudioContext::~FMiniaudioContext()
    {
        for (FPlayingSound& Playing : PlayingSounds)
        {
            ma_sound_uninit(&Playing.Sound);
            ma_decoder_uninit(&Playing.Decoder);
        }
        
        ma_engine_uninit(&Engine);
    }

    void FMiniaudioContext::PlaySoundFromFileAtPosition(FStringView File, glm::vec3 Location)
    {
        TVector<uint8> Bytes;
        if (!VFS::ReadFile(Bytes, File))
        {
            return;
        }

        FPlayingSound& Playing = PlayingSounds.emplace_back();
        Playing.Bytes = std::move(Bytes);

        if (ma_decoder_init_memory(Playing.Bytes.data(), Playing.Bytes.size(), nullptr, &Playing.Decoder) != MA_SUCCESS)
        {
            PlayingSounds.pop_back();
            return;
        }

        if (ma_sound_init_from_data_source(&Engine, &Playing.Decoder, 0, nullptr, &Playing.Sound) != MA_SUCCESS)
        {
            ma_decoder_uninit(&Playing.Decoder);
            PlayingSounds.pop_back();
            return;
        }

        ma_sound_set_spatialization_enabled(&Playing.Sound, MA_TRUE);
        ma_sound_set_position(&Playing.Sound, Location.x, Location.y, Location.z);
        ma_sound_start(&Playing.Sound);
    }

    void FMiniaudioContext::PlaySoundFromFile(FStringView File)
    {
        TVector<uint8> Bytes;
        if (!VFS::ReadFile(Bytes, File))
        {
            return;
        }

        FPlayingSound& Playing = PlayingSounds.emplace_back();
        Playing.Bytes = std::move(Bytes);

        if (ma_decoder_init_memory(Playing.Bytes.data(), Playing.Bytes.size(), nullptr, &Playing.Decoder) != MA_SUCCESS)
        {
            PlayingSounds.pop_back();
            return;
        }

        if (ma_sound_init_from_data_source(&Engine, &Playing.Decoder, 0, nullptr, &Playing.Sound) != MA_SUCCESS)
        {
            ma_decoder_uninit(&Playing.Decoder);
            PlayingSounds.pop_back();
            return;
        }

        //ma_sound_set_spatialization_enabled(&Playing.Sound, MA_TRUE);
        //ma_sound_set_position(&Playing.Sound, Location.x, Location.y, Location.z);
        ma_sound_start(&Playing.Sound);
    }

    void FMiniaudioContext::UpdateListenerPosition(glm::vec3 Location, glm::quat Rotation)
    {
        glm::vec3 Forward = glm::normalize(glm::rotate(Rotation, glm::vec3(0.0f, 0.0f, 1.0f)));
        glm::vec3 Up      = glm::normalize(glm::rotate(Rotation, glm::vec3(0.0f, 1.0f, 0.0f)));

        ma_engine_listener_set_position(&Engine, 0, Location.x, Location.y, Location.z);
        ma_engine_listener_set_direction(&Engine, 0, Forward.x, Forward.y, Forward.z);
        ma_engine_listener_set_world_up(&Engine, 0, Up.x, Up.y, Up.z);
    }

    void FMiniaudioContext::Update()
    {
        LUMINA_PROFILE_SCOPE();

        for (auto It = PlayingSounds.begin(); It != PlayingSounds.end();)
        {
            if (ma_sound_at_end(&It->Sound))
            {
                ma_sound_uninit(&It->Sound);
                ma_decoder_uninit(&It->Decoder);
                It = PlayingSounds.erase(It);
            }
            else
            {
                ++It;
            }
        }
    }

    void FMiniaudioContext::StopSounds()
    {
        for (FPlayingSound& Playing : PlayingSounds)
        {
            ma_sound_stop(&Playing.Sound);
            ma_sound_uninit(&Playing.Sound);
            ma_decoder_uninit(&Playing.Decoder);
        }
        
        PlayingSounds.clear();
    }
}
