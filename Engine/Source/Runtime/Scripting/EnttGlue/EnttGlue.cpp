#include "pch.h"
#include "EnttGlue.h"
#include <vector>

#include "Core/Profiler/Profile.h"
#include "Core/Templates/LuminaTemplate.h"
#include "Log/Log.h"
#include "World/Entity/EntityUtils.h"

namespace Lumina::Scripting::Glue
{
    void RegisterRegistry(sol::table EnttModule)
    {
        using namespace entt::literals;

        EnttModule.new_usertype<entt::entity>("Entity",
        "IsNull",   [] (entt::entity Self) { return Self == entt::null; },
        sol::meta_function::to_string, [](const entt::entity& e)
        {
            return std::to_string(static_cast<uint32>(e));
        });
        
        EnttModule.new_usertype<entt::registry>("Registry",
            sol::meta_function::construct,
            sol::factories([] { return entt::registry{}; }),
            
            "Size",     [](const entt::registry& Self) { return Self.view<entt::entity>()->size(); },
            "Alive",    [](const entt::registry& Self) { return Self.view<entt::entity>()->free_list(); },
            "Orphan",   &entt::registry::orphan,
            "Valid",    &entt::registry::valid,
            "Current",  &entt::registry::current,
            "Create",   [](entt::registry& Self) { return Self.create(); },
            "Destroy",  [](entt::registry& Self, entt::entity Entity) { return Self.destroy(Entity); },
            "Emplace",  [](entt::registry& Self, entt::entity Entity, const sol::table& Component)
            {
                entt::id_type TypeID = ECS::Utils::DeduceType(Component);
                const entt::meta_any& MaybeAny = ECS::Utils::InvokeMetaFunc(TypeID, "emplace_lua"_hs, 
                    entt::forward_as_meta(Self), Entity, entt::forward_as_meta(Component), sol::state_view(Component.lua_state()));
                return MaybeAny ? MaybeAny.cast<sol::reference>() : sol::nil;
            },
            "Remove",   [](entt::registry& Self, entt::entity Entity, const sol::object& Component)
            {
                entt::id_type TypeID = ECS::Utils::DeduceType(Component);
                ECS::Utils::InvokeMetaFunc(TypeID, "remove"_hs, entt::forward_as_meta(Self), Entity);
            },
            "Has",      [](entt::registry& Self, entt::entity Entity, const sol::object& Component)
            {
                entt::meta_any Any = ECS::Utils::InvokeMetaFunc(ECS::Utils::DeduceType(Component), "has"_hs, entt::forward_as_meta(Self), Entity);
                return Any ? Any.cast<bool>() : false;
            },
            "AnyOf",    [](const entt::registry& Self, entt::entity Entity, const sol::variadic_args& Args)
            {
                return eastl::any_of(Args.cbegin(), Args.cend(), [&](const sol::object& Object)
                {
                    entt::meta_any Any = ECS::Utils::InvokeMetaFunc(ECS::Utils::DeduceType(Object), "has"_hs, entt::forward_as_meta(Self), Entity);
                    return Any ? Any.cast<bool>() : false;
                });
            },
            "AllOf",    [](const entt::registry& Self, entt::entity Entity, const sol::variadic_args& Args)
            {
                return eastl::all_of(Args.cbegin(), Args.cend(), [&](const sol::object& Object)
                {
                    entt::meta_any Any = ECS::Utils::InvokeMetaFunc(ECS::Utils::DeduceType(Object), "has"_hs, entt::forward_as_meta(Self), Entity);
                    return Any ? Any.cast<bool>() : false;
                });
            },
            "Get",      [](const entt::registry& Self, entt::entity Entity, const sol::variadic_args& Args)
            {
                sol::variadic_results Results;
                Results.reserve(Args.size());
                
                entt::id_type FunctionID = "get_lua"_hs;
                sol::state_view LuaState(Args.lua_state());
                
                for (const sol::object Proxy : Args)
                {
                    entt::id_type TypeID = ECS::Utils::DeduceType(Proxy);
                    if (const entt::meta_any& MaybeAny = ECS::Utils::InvokeMetaFunc(TypeID, FunctionID, entt::forward_as_meta(Self), Entity, LuaState))
                    {
                        Results.emplace_back(MaybeAny.cast<sol::reference>());
                    }
                    else
                    {
                        Results.emplace_back(sol::nil);
                    }
                }
                
                return Results;
            },
            "TryGet",   [](const entt::registry& Self, entt::entity Entity, const sol::variadic_args& Args)
            {
                sol::variadic_results Results;
                Results.reserve(Args.size());
                
                entt::id_type FunctionID = "try_get_lua"_hs;
                sol::state_view LuaState(Args.lua_state());
                
                for (const sol::object Proxy : Args)
                {
                    entt::id_type TypeID = ECS::Utils::DeduceType(Proxy);
                    if (const entt::meta_any& MaybeAny = ECS::Utils::InvokeMetaFunc(TypeID, FunctionID, entt::forward_as_meta(Self), Entity, LuaState))
                    {
                        sol::object Object = MaybeAny.cast<sol::object>();
                        Results.emplace_back(Object.valid() ? Move(Object) : sol::nil);
                    }
                    else
                    {
                        Results.emplace_back(sol::nil);
                    }
                }
                return Results;
        
            },
            "Clear",    [](const entt::registry& Self, const sol::variadic_args& Args)
            {
                entt::id_type FunctionID = "clear"_hs;
                for (const sol::object Proxy : Args)
                {
                    entt::id_type TypeID = ECS::Utils::DeduceType(Proxy);
                    ECS::Utils::InvokeMetaFunc(TypeID, FunctionID, entt::forward_as_meta(Self));
                }
            },
            "View",     [](entt::registry& Self, const sol::variadic_args& Args)
            {
                entt::runtime_view RuntimeView;
                const THashSet<entt::id_type>& Types = ECS::Utils::CollectTypes(Args);
                for (entt::id_type Type : Types)
                {
                    entt::meta_type Meta = entt::resolve(Type);
                    if (!Meta)
                    {
                        if (entt::basic_sparse_set<>* Storage = Self.storage(Type))
                        {
                            RuntimeView.iterate(*Storage);
                        }
                    }
                    else if (entt::basic_sparse_set<>* Storage = Self.storage(Meta.info().hash()))
                    {
                        RuntimeView.iterate(*Storage);
                    }
                }
                return RuntimeView;
            });

    }

    void RegisterRuntimeView(sol::table EnttModule)
    {
        EnttModule.new_usertype<entt::runtime_view>("View", 
        sol::no_constructor,
        
        
        "SizeHint", &entt::runtime_view::size_hint,
        "Contains", &entt::runtime_view::contains,
        "Each", [](const entt::runtime_view& Self, const sol::function& Callback)
        {
            LUMINA_PROFILE_SECTION("[Lua] RuntimeView::Each");
            if (Callback.valid())
            {
                for (entt::entity Entity : Self)
                {
                    Callback(Entity);
                }
            }
        });
    }
}
