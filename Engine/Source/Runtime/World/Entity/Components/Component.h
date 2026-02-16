#pragma once

#include <sol/sol.hpp>
#include "Core/Engine/Engine.h"
#include "Core/Object/Class.h"
#include "Core/Serialization/Archiver.h"
#include "Traits/ComponentTraits.h"
#include "World/Entity/Traits.h"

namespace Lumina
{
    namespace Meta
    {
        template<typename TComponent>
        bool HasComponent(entt::registry& Registry, entt::entity Entity)
        {
            return Registry.any_of<TComponent>(Entity);
        }

        template<typename TComponent>
        auto RemoveComponent(entt::registry& Registry, entt::entity Entity)
        {
            return Registry.remove<TComponent>(Entity);
        }

        template<typename TComponent>
        void ClearComponent(entt::registry& Registry)
        {
            Registry.clear<TComponent>();
        }

        template<typename TComponent>
        decltype(auto) EmplaceComponent(entt::registry& Registry, entt::entity Entity, const entt::meta_any& Any)
        {
            if constexpr (eastl::is_empty_v<TComponent>)
            {
                Registry.emplace<TComponent>(Entity);
            }
            else
            {
                return Registry.emplace_or_replace<TComponent>(Entity, Any ? Any.cast<const TComponent&>() : TComponent{});
            }
        }
        
        template<typename TComponent>
        TComponent& PatchComponent(entt::registry& Registry, entt::entity Entity, const entt::meta_any& Any)
        {
            return Registry.patch<TComponent>(Entity, [&](TComponent& Type)
            {
                Type = Any.cast<const TComponent&>();
            });
        }

        template<typename TComponent>
        TComponent& GetComponent(entt::registry& Registry, entt::entity Entity)
        {
            return Registry.get<TComponent>(Entity);
        }

        template<typename TComponent>
        void Serialize(FArchive& Ar, entt::meta_any& Any)
        {
            CStruct* Struct = TComponent::StaticStruct();
            TComponent& Instance = Any.cast<TComponent&>();
            Struct->SerializeTaggedProperties(Ar, &Instance);
        }
        
        template<typename TComponent>
        CStruct* GetStructType()
        {
            return TComponent::StaticStruct();
        }

        // Begin Lua variants
    
        template<typename TComponent>
        sol::reference EmplaceComponentLua(entt::registry& Registry, entt::entity Entity, const sol::table& Instance, sol::state_view S)
        {
            auto& Component = Registry.emplace<TComponent>(Entity, Instance.valid() ? Move(Instance.as<TComponent&&>()) : TComponent{});
            return sol::make_reference(S, std::ref(Component));
        }
        
        template<typename TComponent>
        sol::reference PatchComponentLua(entt::registry& Registry, entt::entity Entity, const sol::table& Instance, sol::state_view S)
        {
            TComponent& Component = Registry.emplace<TComponent>(Entity, Move(Instance.as<TComponent&&>()));
            Registry.patch<TComponent>(Entity, [&](TComponent& Type)
            {
                Type = Component;
            });
            
            return sol::make_reference(S, std::ref(Component));
        }
    
        template<typename TComponent>
        sol::reference GetComponentLua(entt::registry& Registry, entt::entity Entity, sol::state_view S)
        {
            LUMINA_PROFILE_SCOPE();
            return sol::make_reference(S, std::ref(Registry.get<TComponent>(Entity)));
        }
        
        template<typename TComponent>
        sol::object TryGetComponentLua(entt::registry& Registry, entt::entity Entity, sol::state_view S)
        {
            auto* Component = Registry.try_get<TComponent>(Entity);
            return Component ? sol::make_reference(S, std::ref(*Component)) : sol::nil;
        }
        
        template<typename TComponent>
        auto OnConstruct_Lua(entt::registry& Registry, const sol::function& Function)
        {
            struct FScriptListener
            {
                FScriptListener(entt::registry& Registry, const sol::function& Function)
                    : Callback(Function)
                {
                    Connection = Registry.on_construct<TComponent>().template connect<&FScriptListener::Receive>(*this);
                }
                
                ~FScriptListener()
                {
                    Connection.release();
                    Callback.abandon();
                }
                
                LE_NO_COPYMOVE(FScriptListener);
                
                void Receive(entt::registry& Registry, entt::entity Entity)
                {
                    if (Connection && Callback.valid())
                    {
                        TComponent& NewComponent = Registry.get<TComponent>(Entity);
                        Callback(Entity, NewComponent);
                    }
                }
                
                sol::function Callback;
                entt::connection Connection;
            };
            
            return Function.lua_state(), std::make_unique<FScriptListener>(Registry, Function);
        }
        
        template<typename TComponent>
        auto OnDestroy_Lua(entt::registry& Registry, const sol::function& Function)
        {
            struct FScriptListener
            {
                FScriptListener(entt::registry& Registry, const sol::function& Function)
                    : Callback(Function)
                {
                    Connection = Registry.on_destroy<TComponent>().template connect<&FScriptListener::Receive>(*this);
                }
                
                ~FScriptListener()
                {
                    Connection.release();
                    Callback.abandon();
                }
                
                LE_NO_COPYMOVE(FScriptListener);
                
                void Receive(entt::registry& Registry, entt::entity Entity)
                {
                    if (Connection && Callback.valid())
                    {
                        TComponent& NewComponent = Registry.get<TComponent>(Entity);
                        Callback(Entity, NewComponent);
                    }
                }
                
                sol::function Callback;
                entt::connection Connection;
            };
            
            return Function.lua_state(), std::make_unique<FScriptListener>(Registry, Function);
        }
        
        template<typename TEvent>
        auto ConnectListener_Lua(entt::dispatcher& Dispatcher, const sol::function& Function)
        {
            struct FScriptListener
            {
                FScriptListener(entt::dispatcher& Dispatcher, const sol::function& Function)
                    : Callback(Function)
                {
                    Connection = Dispatcher.sink<TEvent>().template connect<&FScriptListener::Receive>(*this);
                }
                
                ~FScriptListener()
                {
                    Connection.release();
                    Callback.abandon();
                }
                
                LE_NO_COPYMOVE(FScriptListener);
                
                void Receive(const TEvent& Event)
                {
                    if (Connection && Callback.valid())
                    {
                        Callback(Event);
                    }
                }
                
                sol::function Callback;
                entt::connection Connection;
            };
            
            return Function.lua_state(), std::make_unique<FScriptListener>(Dispatcher, Function);
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
        
        // End lua variants
        
        template<typename TComponent>
        void RegisterComponentMeta()
        {
            using namespace entt::literals;
            auto Meta = entt::meta_factory<TComponent>(GEngine->GetEngineMetaContext())
                .type(TComponent::StaticStruct()->GetName().c_str())
                .traits(ECS::ETraits::Component)
                .template func<&GetStructType<TComponent>>("static_struct"_hs);
            
            Meta.template func<&RemoveComponent<TComponent>>("remove"_hs)
            .template func<&ClearComponent<TComponent>>("clear"_hs)
            .template func<&EmplaceComponent<TComponent>>("emplace"_hs)
            .template func<&HasComponent<TComponent>>("has"_hs);
            
            if constexpr (!eastl::is_empty_v<TComponent>)
            {
                Meta.template func<&GetComponent<TComponent>>("get"_hs);
                Meta.template func<&PatchComponentLua<TComponent>>("patch_lua"_hs);
                Meta.template func<&GetComponentLua<TComponent>>("get_lua"_hs);
                Meta.template func<&TryGetComponentLua<TComponent>>("try_get_lua"_hs);
                Meta.template func<&OnConstruct_Lua<TComponent>>("on_construct_lua"_hs);
                Meta.template func<&OnDestroy_Lua<TComponent>>("on_destroy_lua"_hs);
                Meta.template func<&EmplaceComponentLua<TComponent>>("emplace_lua"_hs);
                Meta.template func<&PatchComponent<TComponent>>("patch"_hs);
                Meta.template func<&Serialize<TComponent>>("serialize"_hs);
                Meta.template func<&ConnectListener_Lua<TComponent>>("connect_listener_lua"_hs);
                Meta.template func<&TriggerEvent_Lua<TComponent>>("trigger_event_lua"_hs);
            }
            
        }
    }
}
