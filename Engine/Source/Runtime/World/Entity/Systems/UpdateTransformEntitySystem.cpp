#include "pch.h"
#include "UpdateTransformEntitySystem.h"
#include "glm/gtx/string_cast.hpp"
#include "TaskSystem/TaskSystem.h"
#include "World/Entity/EntityUtils.h"
#include "World/Entity/Components/CameraComponent.h"
#include "World/Entity/Components/DirtyComponent.h"
#include "World/Entity/Components/RelationshipComponent.h"
#include "World/Entity/Components/Transformcomponent.h"

namespace Lumina
{
    void SUpdateTransformEntitySystem::Update(const FSystemContext& SystemContext) noexcept
    {
        LUMINA_PROFILE_SCOPE();

        auto SingleView = SystemContext.CreateView<FNeedsTransformUpdate, STransformComponent>(entt::exclude<FRelationshipComponent>);
        auto RelationshipGroup = SystemContext.CreateGroup<FNeedsTransformUpdate, FRelationshipComponent>(entt::get<STransformComponent>);
        
        if (!RelationshipGroup.empty())
        {
            TFixedVector<entt::entity, 2000> DirtyEntities;
            DirtyEntities.reserve(RelationshipGroup.size());
            for (auto entity : RelationshipGroup)
            {
                DirtyEntities.push_back(entity);
            }
            
            auto RelationshipTransformCallable = [&](uint32 Index)
            {
                entt::entity DirtyEntity = DirtyEntities[Index];
                
                auto& DirtyTransform = RelationshipGroup.get<STransformComponent>(DirtyEntity);
                auto& DirtyRelationship = RelationshipGroup.get<FRelationshipComponent>(DirtyEntity);
                
                if (DirtyRelationship.Parent != entt::null && SystemContext.IsValidEntity(DirtyRelationship.Parent))
                {
                    glm::mat4 ParentWorldTransform         = SystemContext.Get<STransformComponent>(DirtyRelationship.Parent).WorldTransform.GetMatrix();
                    glm::mat4 LocalTransform               = DirtyTransform.Transform.GetMatrix();
                    DirtyTransform.WorldTransform          = FTransform(ParentWorldTransform * LocalTransform);
                }
                else
                {
                    DirtyTransform.WorldTransform = DirtyTransform.Transform;
                }
                
                DirtyTransform.CachedMatrix = DirtyTransform.WorldTransform.GetMatrix();
                
                TFunction<void(entt::entity)> UpdateChildrenRecursive;
                UpdateChildrenRecursive = [&](entt::entity ParentEntity)
                {
                    ECS::Utils::ForEachChild(SystemContext.GetRegistry(), ParentEntity, [&](entt::entity Child)
                    {
                        auto& ParentTransform = SystemContext.Get<STransformComponent>(ParentEntity);
                        auto& ChildTransform = SystemContext.Get<STransformComponent>(Child);

                        glm::mat4 ParentWorldTransform = ParentTransform.WorldTransform.GetMatrix();
                        glm::mat4 ChildLocalTransform = ChildTransform.Transform.GetMatrix();
                        
                        ChildTransform.WorldTransform = FTransform(ParentWorldTransform * ChildLocalTransform);
                        ChildTransform.CachedMatrix = ChildTransform.WorldTransform.GetMatrix();
                        
                        UpdateChildrenRecursive(Child);
                    });
                };
                
                UpdateChildrenRecursive(DirtyEntity);
            };
            
            if (DirtyEntities.size() > 1000)
            {
                Task::ParallelFor((uint32)DirtyEntities.size(), RelationshipTransformCallable);
            }
            else
            {
                for (uint32 i = 0; i < (uint32)DirtyEntities.size(); ++i)
                {
                    RelationshipTransformCallable(i);
                }
            }
        }

        if (SingleView.size_hint() < 1000)
        {
            SingleView.each([&](FNeedsTransformUpdate&, STransformComponent& TransformComponent)
            {
                TransformComponent.WorldTransform = TransformComponent.Transform;
                TransformComponent.CachedMatrix = TransformComponent.WorldTransform.GetMatrix();  
            });
        }
        else
        {
            auto WorkFunctor = [&](FNeedsTransformUpdate&, STransformComponent& Transform)
            {
                Transform.WorldTransform = Transform.Transform;
                Transform.CachedMatrix = Transform.Transform.GetMatrix();
            };

            auto Handle = SingleView.handle();
            Task::ParallelFor(Handle->size(), [&](uint32 Index)
            {
                entt::entity Entity = (*Handle)[Index];
                
                if (SingleView.contains(Entity))
                {
                    std::apply(WorkFunctor, SingleView.get(Entity));
                }
            });
        }
        
        auto View = SystemContext.CreateView<SCameraComponent, STransformComponent>();
        View.each([](SCameraComponent& CameraComponent, const STransformComponent& TransformComponent)
        {
            CameraComponent.SetView(TransformComponent.WorldTransform.Location, TransformComponent.GetForward(), TransformComponent.GetUp());
        });
        
        SystemContext.Clear<FNeedsTransformUpdate>();
    }
}
