#pragma once

#include "entt/entt.hpp"
#include "Core/Engine/Engine.h"
#include "World/Entity/Traits.h"


namespace Lumina::Meta
{
    class CStruct;

    template<typename TEvent>
    CStruct* GetStructType()
    {
        return TEvent::StaticStruct();
    }
    
    
    template<typename TEvent>
    void RegisterECSEvent()
    {
        using namespace entt::literals;
        auto Meta = entt::meta_factory<TEvent>(GEngine->GetEngineMetaContext())
            .type(TEvent::StaticStruct()->GetName().c_str())
            .traits(ECS::ETraits::Event);
    }
}
