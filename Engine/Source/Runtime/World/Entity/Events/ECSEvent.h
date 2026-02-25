#pragma once

#include <sol/sol.hpp>
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
    
    template <typename TEvent>
    void TriggerEvent_Lua(entt::dispatcher& Dispatcher, const sol::object& Event) 
    {
        Dispatcher.trigger(Event.as<TEvent>());
    }
        
    template <typename TEvent>
    void EnqueueEvent_Lua(entt::dispatcher& Dispatcher, const sol::object& Event) 
    {
        Dispatcher.enqueue(Event.as<TEvent>());
    }
    
    template<typename TEvent>
    void RegisterECSEvent()
    {
        using namespace entt::literals;
        auto Meta = entt::meta_factory<TEvent>(GEngine->GetEngineMetaContext())
            .type(TEvent::StaticStruct()->GetName().c_str())
            .traits(ECS::ETraits::Event);
        
        //Meta.template func<&GetStructType<TEvent>>("static_struct"_hs);
        Meta.template func<&TriggerEvent_Lua<TEvent>>("trigger_event_lua"_hs);
        Meta.template func<&EnqueueEvent_Lua<TEvent>>("enqueue_event_lua"_hs);
    }
}
