#include "pch.h"
#include "SystemContext.h"
#include "sol/sol.hpp"
#include "World/World.h"
#include "World/Entity/EntityUtils.h"
#include "World/Entity/Components/DirtyComponent.h"
#include "world/entity/components/namecomponent.h"

namespace Lumina
{
    FSystemContext::FSystemContext(CWorld* InWorld)
        : World(InWorld)
        , Registry(InWorld->EntityRegistry)
        , Dispatcher(InWorld->SingletonDispatcher)
    {}

    void FSystemContext::RegisterWithLua(sol::state& Lua)
    {
        Lua.new_usertype<FSystemContext>("SystemContext",
            sol::no_constructor,
            "GetDeltaTime",         &FSystemContext::GetDeltaTime,
            "GetTime",              &FSystemContext::GetTime,
            "GetUpdateStage",       &FSystemContext::GetUpdateStage,
            
            "DrawDebugLine",        &FSystemContext::DrawDebugLine,
            "DrawDebugBox",         &FSystemContext::DrawDebugBox,
            "DrawDebugSphere",      &FSystemContext::DrawDebugSphere,
            "DrawDebugArrow",       &FSystemContext::DrawDebugArrow,
            
            "ActivateBody",         &FSystemContext::ActivateBody,
            "DeactivateBody",       &FSystemContext::DeactivateBody,
            "ChangeBodyMotionType", &FSystemContext::ChangeBodyMotionType,

            "GetRegistry",          &FSystemContext::GetRegistry,
            "GetNumEntities",       &FSystemContext::GetNumEntities,
            "IsValidEntity",        &FSystemContext::IsValidEntity,
            "GetTransform",         &FSystemContext::GetEntityTransform,
            "Destroy",              &FSystemContext::Destroy,
            "View",                 &FSystemContext::Lua_View,
            "AllOf",                &FSystemContext::Lua_HasAllOf,
            "AnyOf",                &FSystemContext::Lua_HasAnyOf,
            "OnConstruct",          &FSystemContext::Lua_OnConstruct,
            "ConnectEvent",         &FSystemContext::Lua_ConnectEvent,
            "DispatchEvent",        &FSystemContext::Lua_DispatchEvent,
            "Emplace",              &FSystemContext::Lua_Emplace,
            "GetUnsafe",            &FSystemContext::Lua_GetUnsafe,
            "Get",                  &FSystemContext::Lua_Get,
            "GetByTag",             &FSystemContext::Lua_GetEntityByTag,
            "GetByName",            &FSystemContext::Lua_GetEntityByName,
            "GetFirstWith",         &FSystemContext::Lua_GetFirstEntityWith,

            "Remove",               &FSystemContext::Lua_Remove,
            "SetActiveCamera",      &FSystemContext::Lua_SetActiveCamera,
            "TranslateEntity",      &FSystemContext::TranslateEntity,
            "SetEntityLocation",    &FSystemContext::SetEntityLocation,
            "SetEntityRotation",    &FSystemContext::SetEntityRotation,
            "SetEntityScale",       &FSystemContext::SetEntityScale,
            
            
            "Create",               sol::overload(
                [&](const FSystemContext& Self) { return Self.Create(); },
                [&](const FSystemContext& Self, const glm::vec3& Location) { return Self.Create(Location); }),
            
            "DirtyTransform",       sol::overload(
                [&](FSystemContext& Self, entt::entity Entity)
                {
                    Self.MarkEntityTransformDirty(Entity);
                },
                [&](FSystemContext& Self, entt::entity Entity, EMoveMode MoveMode)
                {
                    Self.MarkEntityTransformDirty(Entity, MoveMode);
                },
                [&](FSystemContext& Self, entt::entity Entity, EMoveMode MoveMode, bool bActivate)
                {
                    Self.MarkEntityTransformDirty(Entity, MoveMode, bActivate);
                }),
                
            "CastRay",              sol::overload(
                [&](FSystemContext& Self, const glm::vec3& Start, const glm::vec3& End, bool bDrawDebug, float DebugDuration)
                {
                    TOptional<FRayResult> Result = Self.CastRay(Start, End, bDrawDebug, DebugDuration);
                    return Result.has_value() ? sol::make_optional(Result.value()) : sol::nullopt;
                },
                [&](FSystemContext& Self, const glm::vec3& Start, const glm::vec3& End, bool bDrawDebug, float DebugDuration, uint32 LayerMask)
                {
                    TOptional<FRayResult> Result = Self.CastRay(Start, End, bDrawDebug, DebugDuration, LayerMask);
                    return Result.has_value() ? sol::make_optional(Result.value()) : sol::nullopt;
                },
                [&](FSystemContext& Self, const glm::vec3& Start, const glm::vec3& End, bool bDrawDebug, float DebugDuration, uint32 LayerMask, uint32 IgnoreBody)
                {
                    TOptional<FRayResult> Result = Self.CastRay(Start, End, bDrawDebug, DebugDuration, LayerMask, IgnoreBody);
                    return Result.has_value() ? sol::make_optional(Result.value()) : sol::nullopt;
                }),

                
            "CastSphere",           &FSystemContext::CastSphere);
    }
    
    entt::runtime_view FSystemContext::CreateRuntimeView(const THashSet<entt::id_type>& Components) const
    {
        entt::runtime_view RuntimeView;
        
        for (entt::id_type Type : Components)
        {
            entt::meta_type Meta = entt::resolve(Type);
            if (!Meta)
            {
                if (entt::basic_sparse_set<>* Storage = Registry.storage(Type))
                {
                    RuntimeView.iterate(*Storage);
                }
            }
            else if (entt::basic_sparse_set<>* Storage = Registry.storage(Meta.info().hash()))
            {
                RuntimeView.iterate(*Storage);
            }
        }

        return RuntimeView;
    }

    void FSystemContext::ActivateBody(uint32 BodyID)
    {
        World->PhysicsScene->ActivateBody(BodyID);
    }

    void FSystemContext::DeactivateBody(uint32 BodyID)
    {
        World->PhysicsScene->DeactivateBody(BodyID);
    }

    void FSystemContext::ChangeBodyMotionType(uint32 BodyID, EBodyType NewType)
    {
        World->PhysicsScene->ChangeBodyMotionType(BodyID, NewType);
    }

    TOptional<FRayResult> FSystemContext::CastRay(const glm::vec3& Start, const glm::vec3& End, bool bDrawDebug, float DebugDuration, uint32 LayerMask, int64 IgnoreBody) const
    {
        return World->CastRay(Start, End, bDrawDebug, DebugDuration, LayerMask, IgnoreBody);
    }

    TVector<FRayResult> FSystemContext::CastSphere(const FSphereCastSettings& Settings) const
    {
        return World->CastSphere(Settings);
    }

    STransformComponent& FSystemContext::GetEntityTransform(entt::entity Entity) const
    {
        return Get<STransformComponent>(Entity);
    }

    glm::vec3 FSystemContext::TranslateEntity(entt::entity Entity, const glm::vec3& Translation)
    {
        glm::vec3 NewLocation = Registry.get<STransformComponent>(Entity).Translate(Translation);
        MarkEntityTransformDirty(Entity);
        return NewLocation;
    }

    void FSystemContext::SetEntityLocation(entt::entity Entity, const glm::vec3& Location)
    {
        Registry.get<STransformComponent>(Entity).SetLocation(Location);
        MarkEntityTransformDirty(Entity);
    }

    void FSystemContext::SetEntityRotation(entt::entity Entity, const glm::quat& Rotation)
    {
        Registry.get<STransformComponent>(Entity).SetRotation(Rotation);
        MarkEntityTransformDirty(Entity);
    }

    void FSystemContext::SetEntityScale(entt::entity Entity, const glm::vec3& Scale)
    {
        Registry.get<STransformComponent>(Entity).SetScale(Scale);
        MarkEntityTransformDirty(Entity);
    }

    void FSystemContext::MarkEntityTransformDirty(entt::entity Entity, EMoveMode MoveMode, bool bActivate)
    {
        EmplaceOrReplace<FNeedsTransformUpdate>(Entity, FNeedsTransformUpdate{MoveMode, bActivate});  
    }

    void FSystemContext::DrawDebugLine(const glm::vec3& Start, const glm::vec3& End, const glm::vec4& Color, float Thickness, float Duration)
    {
        World->DrawLine(Start, End, Color, Thickness, true, Duration);
    }

    void FSystemContext::DrawDebugBox(const glm::vec3& Center, const glm::vec3& Extents, const glm::quat& Rotation, const glm::vec4& Color, float Thickness, float Duration)
    {
        World->DrawBox(Center, Extents, Rotation, Color, Thickness, true, Duration);
    }

    void FSystemContext::DrawDebugSphere(const glm::vec3& Center, float Radius, const glm::vec4& Color, uint8 Segments, float Thickness, float Duration)
    {
        World->DrawSphere(Center, Radius, Color, Segments, Thickness, true, Duration);
    }

    void FSystemContext::DrawDebugCone(const glm::vec3& Apex, const glm::vec3& Direction, float AngleRadians, float Length, const glm::vec4& Color, uint8 Segments, uint8 Stacks, float Thickness, float Duration)
    {
        World->DrawCone(Apex, Direction, AngleRadians, Length, Color, Segments, Stacks, Thickness, true, Duration);
    }

    void FSystemContext::DrawFrustum(const glm::mat4& Matrix, float zNear, float zFar, const glm::vec4& Color, float Thickness, float Duration)
    {
        World->DrawFrustum(Matrix, zNear, zFar, Color, Thickness, true, Duration);
    }

    void FSystemContext::DrawDebugArrow(const glm::vec3& Start, const glm::vec3& Direction, float Length, const glm::vec4& Color, float Thickness, float Duration, float HeadSize)
    {
        World->DrawArrow(Start, Direction, Length, Color, Thickness, true, Duration, HeadSize);
    }
    
    entt::entity FSystemContext::Create(glm::vec3 Location) const
    {
        entt::entity EntityID = Registry.create();
        Registry.emplace<STransformComponent>(EntityID).SetLocation(Location);
        Registry.emplace<SNameComponent>(EntityID).Name = "Entity";
        Registry.emplace_or_replace<FNeedsTransformUpdate>(EntityID);
        return EntityID;
    }
    
    entt::entity FSystemContext::Create() const
    {
        entt::entity EntityID = Registry.create();
        Registry.emplace<STransformComponent>(EntityID);
        Registry.emplace<SNameComponent>(EntityID).Name = "Entity";
        Registry.emplace_or_replace<FNeedsTransformUpdate>(EntityID);
        return EntityID;
    }

    size_t FSystemContext::GetNumEntities() const
    {
        return Registry.storage<entt::entity>().size();
    }

    bool FSystemContext::IsValidEntity(entt::entity Entity) const
    {
        return Registry.valid(Entity);
    }

    void FSystemContext::Lua_DispatchEvent(const sol::object& Event) const
    {
        using namespace entt::literals;

        if (const entt::id_type EventID = ECS::Utils::DeduceType(Event))
        {
            ECS::Utils::InvokeMetaFunc(EventID, "trigger_event_lua"_hs, entt::forward_as_meta(Dispatcher), Event);
        }
    }

    entt::meta_any FSystemContext::Lua_ConnectEvent(const sol::object& Event, const sol::function& Listener) const
    {
        using namespace entt::literals;
        
        if (!Listener.valid())
        {
            return entt::meta_any{};
        }
        
        if (const entt::id_type EventID = ECS::Utils::DeduceType(Event))
        {
            return ECS::Utils::InvokeMetaFunc(EventID, "connect_listener_lua"_hs, entt::forward_as_meta(Dispatcher), Listener);
        }
        
        return entt::meta_any{};
    }

    entt::meta_any FSystemContext::Lua_OnConstruct(const sol::object& Event, const sol::function& Listener) const
    {
        using namespace entt::literals;
        
        if (!Listener.valid())
        {
            return entt::meta_any{};
        }
        
        if (const entt::id_type EventID = ECS::Utils::DeduceType(Event))
        {
            return ECS::Utils::InvokeMetaFunc(EventID, "on_construct_lua"_hs, entt::forward_as_meta(Registry), Listener);
        }
        
        return entt::meta_any{};
    }

    bool FSystemContext::Lua_HasAllOf(entt::entity Entity, const sol::variadic_args& Args) const
    {
        return eastl::all_of(Args.cbegin(), Args.cend(), [&](const sol::object& Object)
        {
            return Lua_Has(Entity, Object);
        });
    }

    bool FSystemContext::Lua_HasAnyOf(entt::entity Entity, const sol::variadic_args& Args) const
    {
        return eastl::any_of(Args.cbegin(), Args.cend(), [&](const sol::object& Object)
        {
            return Lua_Has(Entity, Object);
        });
    }

    bool FSystemContext::Lua_Has(entt::entity Entity, const sol::object& Type) const
    {
        using namespace entt::literals;
        entt::meta_any Any = ECS::Utils::InvokeMetaFunc(ECS::Utils::DeduceType(Type), "has"_hs, entt::forward_as_meta(Registry), Entity);
        return Any ? Any.cast<bool>() : false;
    }

    entt::runtime_view FSystemContext::Lua_View(const sol::variadic_args& Args) const
    {
        const THashSet<entt::id_type>& Types = ECS::Utils::CollectTypes(Args);
        return CreateRuntimeView(Types);
    }

    void FSystemContext::Lua_SetActiveCamera(uint32 Entity) const
    {
        Dispatcher.trigger<FSwitchActiveCameraEvent>(FSwitchActiveCameraEvent{(entt::entity)Entity});
    }

    void FSystemContext::Lua_Remove(entt::entity Entity, const sol::object& Component) const
    {
        LUMINA_PROFILE_SCOPE();

        using namespace entt::literals;

        entt::id_type TypeID = ECS::Utils::DeduceType(Component);
        ECS::Utils::InvokeMetaFunc(TypeID, "remove"_hs, entt::forward_as_meta(Registry), Entity);
    }

    sol::object FSystemContext::Lua_Emplace(entt::entity Entity, const sol::table& Component) const
    {
        LUMINA_PROFILE_SCOPE();

        using namespace entt::literals;

        entt::id_type TypeID = ECS::Utils::DeduceType(Component);
        const entt::meta_any& MaybeAny = ECS::Utils::InvokeMetaFunc(TypeID, "emplace_lua"_hs, 
            entt::forward_as_meta(Registry), Entity, entt::forward_as_meta(Component), sol::state_view(Component.lua_state()));

        return MaybeAny ? MaybeAny.cast<sol::reference>() : sol::nil;
    }

    sol::variadic_results FSystemContext::Lua_GetUnsafe(entt::entity Entity, const sol::variadic_args& Args) const
    {
        LUMINA_PROFILE_SCOPE();

        using namespace entt::literals;
        
        sol::variadic_results Results;
        Results.reserve(Args.size());
        
        entt::id_type FunctionID = "get_lua"_hs;
        sol::state_view LuaState(Args.lua_state());
        
        for (const sol::object Proxy : Args)
        {
            entt::id_type TypeID = ECS::Utils::DeduceType(Proxy);
            if (const entt::meta_any& MaybeAny = ECS::Utils::InvokeMetaFunc(TypeID, FunctionID, entt::forward_as_meta(Registry), Entity, LuaState))
            {
                Results.emplace_back(MaybeAny.cast<sol::reference>());
            }
            else
            {
                Results.emplace_back(sol::nil);
            }
        }
        
        return Results;
    }

    sol::variadic_results FSystemContext::Lua_Get(entt::entity Entity, const sol::variadic_args& Args) const
    {
        LUMINA_PROFILE_SCOPE();

        using namespace entt::literals;
        
        sol::variadic_results Results;
        Results.reserve(Args.size());
        
        entt::id_type FunctionID = "try_get_lua"_hs;
        sol::state_view LuaState(Args.lua_state());
        
        for (const sol::object Proxy : Args)
        {
            entt::id_type TypeID = ECS::Utils::DeduceType(Proxy);
            if (const entt::meta_any& MaybeAny = ECS::Utils::InvokeMetaFunc(TypeID, FunctionID, entt::forward_as_meta(Registry), Entity, LuaState))
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
    }

    entt::entity FSystemContext::Lua_GetEntityByTag(const char* Tag) const
    {
        return World->GetEntityByTag(Tag);
    }

    entt::entity FSystemContext::Lua_GetEntityByName(const char* Name) const
    {
        return World->GetEntityByName(Name);
    }

    entt::entity FSystemContext::Lua_GetFirstEntityWith(const sol::object& Component) const
    {
        return World->GetFirstEntityWith(ECS::Utils::DeduceType(Component));
    }
}
