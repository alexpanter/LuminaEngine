#include "pch.h"
#include "World.h"
#include "WorldManager.h"
#include "Core/Delegates/CoreDelegates.h"
#include "Core/Engine/Engine.h"
#include "Core/Object/Class.h"
#include "Core/Object/ObjectIterator.h"
#include "Core/Serialization/MemoryArchiver.h"
#include "Core/Serialization/ObjectArchiver.h"
#include "EASTL/sort.h"
#include "Entity/EntityUtils.h"
#include "Entity/Components/DirtyComponent.h"
#include "Entity/Components/EditorComponent.h"
#include "entity/components/entitytags.h"
#include "Entity/Components/InterpolatingMovementComponent.h"
#include "Entity/Components/LineBatcherComponent.h"
#include "Entity/Components/NameComponent.h"
#include "Entity/Components/ScriptComponent.h"
#include "Entity/Components/SingletonEntityComponent.h"
#include "entity/components/tagcomponent.h"
#include "Events/KeyCodes.h"
#include "glm/gtx/matrix_decompose.hpp"
#include "Physics/Physics.h"
#include "Scene/RenderScene/Forward/ForwardRenderScene.h"
#include "Scripting/Lua/Scripting.h"
#include "Subsystems/FCameraManager.h"
#include "World/Entity/Components/RelationshipComponent.h"
#include "World/entity/systems/EntitySystem.h"

namespace Lumina
{
    CWorld::CWorld()
        : SingletonEntity(entt::null)
        , SystemContext(this)
    {
    }

    void CWorld::Serialize(FArchive& Ar)
    {
        CObject::Serialize(Ar);
        
        if (Ar.IsReading())
        {
            ECS::Utils::SerializeRegistry(Ar, RegistryPending);
        }
        else
        {
            ECS::Utils::SerializeRegistry(Ar, EntityRegistry);
        }
    }

    void CWorld::PreLoad()
    {
    }

    void CWorld::PostLoad()
    {
        //...
    }
    
    void CWorld::InitializeWorld(EWorldType InWorldType)
    {
        using namespace entt::literals;
        
        WorldType = InWorldType;
        GWorldManager->AddWorld(this);
        
        EntityRegistry.swap(RegistryPending);
        RegistryPending = {};

        EntityRegistry.ctx().emplace<entt::dispatcher&>(SingletonDispatcher);
        
        if (!EntityRegistry.valid(SingletonEntity))
        {
            SingletonEntity = EntityRegistry.create();
        }
        
        EntityRegistry.emplace<FSingletonEntityTag>(SingletonEntity);
        EntityRegistry.emplace<FHideInSceneOutliner>(SingletonEntity);
        
        PhysicsScene    = Physics::GetPhysicsContext()->CreatePhysicsScene(this);
        CameraManager   = MakeUnique<FCameraManager>(this);

        EntityRegistry.ctx().emplace<Physics::IPhysicsScene*>(PhysicsScene.get());
        EntityRegistry.ctx().emplace<FCameraManager*>(CameraManager.get());
        EntityRegistry.ctx().emplace<FSystemContext&>(SystemContext);
        
        CreateRenderer();
        RegisterSystems();
        
        ForEachUniqueSystem([&](const FSystemVariant& System)
        {
            eastl::visit([&](const auto& Sys) { Sys.Startup(SystemContext); }, System);
        });
        
        EntityRegistry.on_destroy<FRelationshipComponent>().connect<&ThisClass::OnRelationshipComponentDestroyed>(this);
        EntityRegistry.on_construct<SScriptComponent>().connect<&ThisClass::OnScriptComponentCreated>(this);
        EntityRegistry.on_destroy<SScriptComponent>().connect<&ThisClass::OnScriptComponentDestroyed>(this);
        SystemContext.EventSink<FSwitchActiveCameraEvent>().connect<&ThisClass::OnChangeCameraEvent>(this);
        
        auto CameraView = EntityRegistry.view<SCameraComponent>(entt::exclude<SDisabledTag>);
        CameraView.each([&](entt::entity Entity, SCameraComponent& Camera)
        {
           if (Camera.bAutoActivate)
           {
               SingletonDispatcher.trigger<FSwitchActiveCameraEvent>(FSwitchActiveCameraEvent{Entity});
           }
        });
        
        auto ScriptView = EntityRegistry.view<SScriptComponent>(entt::exclude<SDisabledTag>);
        ScriptView.each([&](entt::entity Entity, SScriptComponent&)
        {
           OnScriptComponentCreated(EntityRegistry, Entity); 
        });
        
        auto RootView = EntityRegistry.view<SScriptComponent>(entt::exclude<FRelationshipComponent, SDisabledTag>);
        RootView.each([&](entt::entity RootEntity, SScriptComponent& ScriptComponent)
        {
            ScriptComponent.InvokeScriptFunction("OnReady");
        });
        
        auto RelationshipView = EntityRegistry.view<SScriptComponent, FRelationshipComponent>(entt::exclude<SDisabledTag>);
        RelationshipView.each([&](entt::entity Entity, SScriptComponent& Script, FRelationshipComponent& Relationship)
        {
            if (Relationship.Parent == entt::null)
            {
                ECS::Utils::ForEachDescendantReverse(EntityRegistry, Entity, [&](entt::entity Descendant)
                {
                    if (SScriptComponent* ScriptComp = EntityRegistry.try_get<SScriptComponent>(Descendant))
                    {
                        ScriptComp->InvokeScriptFunction("OnReady");
                    }
                });
                
                Script.InvokeScriptFunction("OnReady");

            }
        });
        
        if (IsGameWorld())
        {
            bPaused = false;
        }
        
        bInitializing = false;
    }
    
    void CWorld::TeardownWorld()
    {
        RegistryPending.clear<>();
        EntityRegistry.clear<>();
        
        EntityRegistry.on_destroy<FRelationshipComponent>().disconnect<&ThisClass::OnRelationshipComponentDestroyed>(this);
        EntityRegistry.on_destroy<SScriptComponent>().disconnect<&ThisClass::OnScriptComponentCreated>(this);
        
        ForEachUniqueSystem([&](const FSystemVariant& System)
        {
            eastl::visit([&](const auto& Sys) { Sys.Teardown(SystemContext); }, System);
        });
        
        PhysicsScene.reset();
        DestroyRenderer();
        
        FCoreDelegates::PostWorldUnload.Broadcast();
        
        GWorldManager->RemoveWorld(this);
        
        Scripting::FScriptingContext::Get().RunGC();
    }
    
    void CWorld::Update(const FUpdateContext& Context)
    {
        LUMINA_PROFILE_SCOPE();

        const EUpdateStage Stage = Context.GetUpdateStage();
        
        if (Stage == EUpdateStage::FrameStart)
        {
            DeltaTime = Context.GetDeltaTime();
            TimeSinceCreation += DeltaTime;
        }
        
        if (Stage == EUpdateStage::DuringPhysics)
        {
            PhysicsScene->Update(Context.GetDeltaTime());
        }

        SystemContext.DeltaTime = DeltaTime;
        SystemContext.Time = TimeSinceCreation;
        SystemContext.UpdateStage = Stage;
        
        TickSystems(SystemContext);
    }

    void CWorld::Paused(const FUpdateContext& Context)
    {
        LUMINA_PROFILE_SCOPE();

        DeltaTime = Context.GetDeltaTime();
        TimeSinceCreation += DeltaTime;
        
        SystemContext.DeltaTime = DeltaTime;
        SystemContext.Time = TimeSinceCreation;
        SystemContext.UpdateStage = EUpdateStage::Paused;

        TickSystems(SystemContext);
    }

    void CWorld::Render(FRenderGraph& RenderGraph)
    {
        LUMINA_PROFILE_SCOPE();

        SCameraComponent* CameraComponent = GetActiveCamera();
        FViewVolume ViewVolume = CameraComponent ? CameraComponent->GetViewVolume() : FViewVolume();
        
        RenderScene->RenderScene(RenderGraph, ViewVolume);
    }
    
    bool CWorld::RegisterSystem(const FSystemVariant& NewSystem)
    {
        const FUpdatePriorityList& PriorityList = eastl::visit([&](const auto& System) { return System.GetUpdatePriorityList(); }, NewSystem);
        
        bool StagesModified[(uint8)EUpdateStage::Max] = {};

        for (uint8 i = 0; i < (uint8)EUpdateStage::Max; ++i)
        {
            if (PriorityList.IsStageEnabled((EUpdateStage)i))
            {
                SystemUpdateList[i].emplace_back(NewSystem);
                StagesModified[i] = true;
            }
        }

        for (uint8 i = 0; i < (uint8)EUpdateStage::Max; ++i)
        {
            if (!StagesModified[i])
            {
                continue;
            }

            auto Predicate = [i](const FSystemVariant& A, const FSystemVariant& B)
            {
                const FUpdatePriorityList& PrioListA = eastl::visit([&](const auto& System) { return System.GetUpdatePriorityList(); }, A);
                const FUpdatePriorityList& PrioListB = eastl::visit([&](const auto& System) { return System.GetUpdatePriorityList(); }, B);
                const uint8 PriorityA = PrioListA.GetPriorityForStage((EUpdateStage)i);
                const uint8 PriorityB = PrioListB.GetPriorityForStage((EUpdateStage)i);
                return PriorityA > PriorityB;
            };

            eastl::sort(SystemUpdateList[i].begin(), SystemUpdateList[i].end(), Predicate);
        }

        return true;
    }

    entt::entity CWorld::ConstructEntity(const FName& Name, const FTransform& Transform)
    {
        entt::entity NewEntity = GetEntityRegistry().create();
        
        FName ActualName = Name;
        if (ActualName == NAME_None)
        {
            FFixedString StringName;
            StringName.append_convert(Name + eastl::to_string(entt::to_integral(NewEntity)));
            ActualName = StringName;
        }
        
        EntityRegistry.emplace<SNameComponent>(NewEntity).Name = Name;
        EntityRegistry.emplace<STransformComponent>(NewEntity).Transform = Transform;
        EntityRegistry.emplace_or_replace<FNeedsTransformUpdate>(NewEntity);
        
        return NewEntity;
    }
    
    void CWorld::CopyEntity(entt::entity& To, entt::entity From, TFunctionRef<bool(entt::type_info)> Callback)
    {
        ASSERT(To != From);
        
        To = EntityRegistry.create();
        
        for (auto&& [ID, Storage]: EntityRegistry.storage())
        {
            if (!Callback(Storage.info()))
            {
                continue;
            }
            
            // We also need to check the entity we're creating, incase another component adds it.
            if(Storage.contains(From) && !Storage.contains(To))
            {
                Storage.push(To, Storage.value(From));
            }
        }
    }

    void CWorld::DestroyEntity(entt::entity Entity)
    {
        EntityRegistry.destroy(Entity);
    }

    uint32 CWorld::GetNumEntities() const
    {
        return (uint32)EntityRegistry.view<entt::entity>().size();
    }

    void CWorld::SetActiveCamera(entt::entity InEntity)
    {
        if (!EntityRegistry.valid(InEntity))
        {
            return;
        }

        if (EntityRegistry.all_of<SCameraComponent>(InEntity))
        {
            CameraManager->SetActiveCamera(InEntity);
        }
    }

    SCameraComponent* CWorld::GetActiveCamera()
    {
        return CameraManager->GetCameraComponent();
    }

    entt::entity CWorld::GetActiveCameraEntity() const
    {
        return CameraManager->GetActiveCameraEntity();
    }

    void CWorld::OnChangeCameraEvent(const FSwitchActiveCameraEvent& Event)
    {
        SetActiveCamera(Event.NewActiveEntity);
    }

    void CWorld::SimulateWorld()
    {
        PhysicsScene->OnWorldSimulate();
    }

    void CWorld::StopSimulation()
    {
        PhysicsScene->OnWorldStopSimulate();
    }
    
    void CWorld::CreateRenderer()
    {
        if (!RenderScene)
        {
            RenderScene = MakeUnique<FForwardRenderScene>(this);
            RenderScene->Init();
            EntityRegistry.ctx().emplace<IRenderScene*>(RenderScene.get());
        }
    }

    void CWorld::DestroyRenderer()
    {
        if (RenderScene)
        {
            RenderScene->Shutdown();
            RenderScene.reset();
        }
    }

    void CWorld::SetActive(bool bNewActive)
    {
        if (bActive != bNewActive)
        {
            bActive = bNewActive;
            
            if (bActive)
            {
                CreateRenderer();       
            }
            else
            {
                DestroyRenderer();
            }
        }
    }

    CWorld* CWorld::DuplicateWorld(CWorld* OwningWorld)
    {
        CPackage* OuterPackage = OwningWorld->GetPackage();
        if (OuterPackage == nullptr)
        {
            return nullptr;
        }

        TVector<uint8> Data;
        FMemoryWriter Writer(Data);
        FObjectProxyArchiver WriterProxy(Writer, true);
        OwningWorld->Serialize(WriterProxy);
        
        FMemoryReader Reader(Data);
        FObjectProxyArchiver ReaderProxy(Reader, true);
        
        CWorld* PIEWorld = NewObject<CWorld>(OF_Transient);
        
        PIEWorld->PreLoad();
        PIEWorld->Serialize(ReaderProxy);
        PIEWorld->PostLoad();
        
        return PIEWorld;
    }

    const TVector<CWorld::FSystemVariant>& CWorld::GetSystemsForUpdateStage(EUpdateStage Stage)
    {
        return SystemUpdateList[static_cast<uint32>(Stage)];
    }

    void CWorld::OnRelationshipComponentDestroyed(entt::registry& Registry, entt::entity Entity)
    {
        ECS::Utils::RemoveFromParent(Registry, Entity);
        ECS::Utils::DestroyEntityHierarchy(Registry, Entity);
    }
    
    void CWorld::OnScriptComponentCreated(entt::registry& Registry, entt::entity Entity)
    {
        SScriptComponent& ScriptComponent = Registry.get<SScriptComponent>(Entity);
        if (!ScriptComponent.ScriptPath.Path.empty())
        {
            if (TSharedPtr<Scripting::FLuaScript> Script = Scripting::FScriptingContext::Get().LoadUniqueScript(ScriptComponent.ScriptPath.Path))
            {
                ScriptComponent.Script = Script;
                
                auto ScriptVarVisitor = [&]<typename T>(TVector<TNamedScriptVar<T>>& Vector)
                {
                    for (const TNamedScriptVar<T>& Var : Vector)
                    {
                        const char* KeyName = Var.Name.c_str();
                        sol::object ExistingValue = Script->ScriptTable[KeyName];
            
                        if (!ExistingValue.valid() || !ExistingValue.is<T>())
                        {
                            continue;
                        }
                        
                        if constexpr (eastl::is_same_v<T, FString>)
                        {
                            Script->ScriptTable[KeyName] = Var.Value.c_str();
                        }
                        else
                        {
                            Script->ScriptTable[KeyName] = Var.Value;
                        }
                    }
                };
        
                eastl::apply([&](auto&... Vectors)
                {
                    (ScriptVarVisitor(Vectors), ...);
                }, ScriptComponent.CustomData);
            }
            
            if (WorldType == EWorldType::Game)
            {
                ScriptComponent.Script->Environment["Entity"]   = Entity;
                ScriptComponent.Script->Environment["Context"]  = std::ref(SystemContext);
                ScriptComponent.InvokeScriptFunction("OnAttach");
                
                if (!bInitializing)
                {
                    ScriptComponent.InvokeScriptFunction("OnReady");
                }
            }
        }
    }

    void CWorld::OnScriptComponentDestroyed(entt::registry& Registry, entt::entity Entity)
    {
        SScriptComponent& ScriptComponent = Registry.get<SScriptComponent>(Entity);
        if (WorldType == EWorldType::Game || WorldType == EWorldType::Simulation)
        {
            if (ScriptComponent.Script != nullptr || !ScriptComponent.Script->ScriptTable.valid())
            {
                ScriptComponent.Script->Environment["Entity"]   = Entity;
                ScriptComponent.Script->Environment["Context"]  = std::ref(SystemContext);
                ScriptComponent.InvokeScriptFunction("OnDetach");
            }
        }
    }

    void CWorld::RegisterSystems()
    {
        using namespace Scripting;
        using namespace entt::literals;

        for (int i = 0; i < (int)EUpdateStage::Max; ++i)
        {
            SystemUpdateList[i].clear();
        }
        
        for (auto&& [_, Meta] : entt::resolve())
        {
            ECS::ETraits Traits = Meta.traits<ECS::ETraits>();
            if (EnumHasAnyFlags(Traits, ECS::ETraits::System))
            {
                FEntitySystemWrapper Wrapper;
                Wrapper.Underlying = Meta;
                Wrapper.Instance = Meta.construct();
                
                FSystemVariant Variant = Wrapper;
                RegisterSystem(Variant);
            }
        }
        
        //FScriptingContext::Get().ForEachScript([&](FName Path, const TSharedPtr<FLuaScript>& Script)
        //{
        //    if (!Script->ScriptTable["Type"].valid() || Script->ScriptTable["Type"] != EScriptType::WorldSystem)
        //    {
        //        return;
        //    }
        //    
        //    FEntityScriptSystem ScriptSystem;
        //    ScriptSystem.WeakScript = Script;
        //    FSystemVariant Variant = Move(ScriptSystem);
        //    RegisterSystem(Variant);
        //});
    }

    void CWorld::DrawBillboard(FRHIImage* Image, const glm::vec3& Location, float Scale)
    {
        RenderScene->DrawBillboard(Image, Location, Scale);
    }

    void CWorld::DrawLine(const glm::vec3& Start, const glm::vec3& End, const glm::vec4& Color, float Thickness, bool bDepthTest, float Duration)
    {
        FLineBatcherComponent& Batcher = GetOrCreateLineBatcher();
        
        float ActualDuration = eastl::max<float>(static_cast<float>(GetWorldDeltaTime()) + LE_KINDA_SORTA_SMALL_NUMBER, Duration);
        Batcher.DrawLine(Start, End, Color, Thickness, bDepthTest, ActualDuration);
    }
    
    TOptional<FRayResult> CWorld::CastRay(const FRayCastSettings& Settings)
    {
        LUMINA_PROFILE_SCOPE();
        
        if (PhysicsScene == nullptr)
        {
            return eastl::nullopt;
        }
        
        TOptional<FRayResult> Result = PhysicsScene->CastRay(Settings);
        
        if (Settings.bDrawDebug)
        {
            if (Result.has_value())
            {
                FRayResult RayResult = Result.value();
                DrawLine(Settings.Start, RayResult.Location, FColor(Settings.DebugMissColor), 1.0f, true, Settings.DebugDuration);
                
                glm::vec3 NormalEnd = RayResult.Location + RayResult.Normal * 0.5f;
                DrawLine(RayResult.Location, NormalEnd, FColor::Blue, 1.0f,true, Settings.DebugDuration);
                
                DrawBox(RayResult.Location, glm::vec3(0.05f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), FColor::Yellow, 1.0, true, Settings.DebugDuration);
                
                DrawLine(RayResult.Location, Settings.End, FColor(Settings.DebugHitColor), 1.0f, true, Settings.DebugDuration);
            }
            else
            {
                DrawLine(Settings.Start, Settings.End, FColor(Settings.DebugMissColor), 1.0f, true, Settings.DebugDuration);
            }
        }
        
        return Move(Result);
    }

    TOptional<FRayResult> CWorld::CastRay(const glm::vec3& Start, const glm::vec3& End, bool bDrawDebug, float DebugDuration, uint32 LayerMask, int64 IgnoreBody)
    {
        FRayCastSettings Settings;
        Settings.DebugDuration = DebugDuration;
        Settings.Start = Start;
        Settings.End = End;
        Settings.LayerMask = LayerMask;
        Settings.bDrawDebug = bDrawDebug;
        Settings.IgnoreBodies.push_back(IgnoreBody);
        
        return CastRay(Settings);
    }
    
    TVector<FRayResult> CWorld::CastSphere(const FSphereCastSettings& Settings)
    {
        LUMINA_PROFILE_SCOPE();

        if (PhysicsScene == nullptr)
        {
            return {};
        }
        
        return PhysicsScene->CastSphere(Settings);
        
    }

    entt::entity CWorld::GetEntityByTag(const FName& Tag)
    {
        auto& Storage = EntityRegistry.storage<STagComponent>(entt::hashed_string(Tag.c_str()));
        if (Storage.empty())
        {
            return entt::null;
        }
        
        return *Storage.data();
    }

    entt::entity CWorld::GetEntityByName(const FName& Name)
    {
        auto View = EntityRegistry.view<SNameComponent>();
        for (entt::entity Entity : View)
        {
            SNameComponent& NameComponent = View.get<SNameComponent>(Entity);
            if (NameComponent.Name == Name)
            {
                return Entity;
            }
        }
        
        return entt::null;
    }

    entt::entity CWorld::GetFirstEntityWith(entt::id_type Type)
    {
        if (!EntityRegistry.storage(Type))
        {
            return entt::null;
        }

        auto storage = EntityRegistry.storage(Type);

        if (storage->empty())
        {
            return entt::null;
        }
        return *storage->data();
    }

    void CWorld::MarkTransformDirty(entt::entity Entity)
    {
        GetEntityRegistry().emplace_or_replace<FNeedsTransformUpdate>(Entity);
    }

    void CWorld::SetEntityTransform(entt::entity Entity, const FTransform& NewTransform)
    {
        EntityRegistry.emplace_or_replace<STransformComponent>(Entity, NewTransform);
        EntityRegistry.emplace_or_replace<FNeedsTransformUpdate>(Entity);
    }

    TVector<entt::entity> CWorld::GetSelectedEntities() const
    {
        auto View = EntityRegistry.view<FSelectedInEditorComponent>();
        TVector<entt::entity> Selections(View.size());
        View.each([&](entt::entity Entity)
        {
           Selections.push_back(Entity); 
        });
        return Selections;
    }

    bool CWorld::IsSelected(entt::entity Entity) const
    {
        return EntityRegistry.any_of<FSelectedInEditorComponent>(Entity);
    }

    void CWorld::TickSystems(FSystemContext& Context)
    {
        auto& SystemVector = SystemUpdateList[(uint32)Context.GetUpdateStage()];
        for(FSystemVariant& SystemVariant : SystemVector)
        {
            eastl::visit([&](auto& System) { System.Update(Context); }, SystemVariant);
        }
    }

    FLineBatcherComponent& CWorld::GetOrCreateLineBatcher()
    {
        return EntityRegistry.get_or_emplace<FLineBatcherComponent>(SingletonEntity);
    }
}
