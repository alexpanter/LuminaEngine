#include "pch.h"
#include "ComponentVisualizer.h"

#include "Core/Math/Color.h"
#include "Paths/Paths.h"
#include "Renderer/PrimitiveDrawInterface.h"
#include "Renderer/RenderResource.h"
#include "Tools/Import/ImportHelpers.h"
#include "Tools/UI/ImGui/ImGuiX.h"
#include "World/Entity/Components/CharacterComponent.h"
#include "world/entity/components/lightcomponent.h"
#include "world/entity/components/physicscomponent.h"
#include "World/Entity/Components/TransformComponent.h"

namespace Lumina
{
    static CComponentVisualizerRegistry* Singleton = nullptr;
    
    
    CComponentVisualizerRegistry& CComponentVisualizerRegistry::Get()
    {
        static std::once_flag Flag;
        std::call_once(Flag, []()
        {
            Singleton = NewObject<CComponentVisualizerRegistry>();
        });

        return *Singleton;
    }

    void CComponentVisualizerRegistry::RegisterComponentVisualizer(CComponentVisualizer* Visualizer)
    {
        if (CStruct* SupportedType = Visualizer->GetSupportedComponentType())
        {
            Visualizers.emplace(SupportedType, Visualizer);
        }
    }

    CComponentVisualizer* CComponentVisualizerRegistry::GetComponentVisualizer(CStruct* Component)
    {
        auto It = Visualizers.find(Component);
        if (It != Visualizers.end())
        {
            return It->second;
        }
        
        return nullptr;
    }

    void CComponentVisualizer::PostCreateCDO()
    {
        CComponentVisualizerRegistry::Get().RegisterComponentVisualizer(this);
    }

    CStruct* CComponentVisualizer_PointLight::GetSupportedComponentType() const
    {
        return SPointLightComponent::StaticStruct();
    }

    void CComponentVisualizer_PointLight::Draw(IPrimitiveDrawInterface* PDI, entt::registry& Registry, entt::entity Entity)
    {
        const SPointLightComponent& PointLight = Registry.get<SPointLightComponent>(Entity);
        const STransformComponent& Transform = Registry.get<STransformComponent>(Entity);
        
        PDI->DrawSphere(Transform.GetLocation(), PointLight.Attenuation, 
            glm::vec4(PointLight.LightColor, 1.0f), 32, 1.0f, true, 0.0f);
    }

    CStruct* CComponentVisualizer_SphereCollider::GetSupportedComponentType() const
    {
        return SSphereColliderComponent::StaticStruct();
    }

    void CComponentVisualizer_SphereCollider::Draw(IPrimitiveDrawInterface* PDI, entt::registry& Registry, entt::entity Entity)
    {
        const SSphereColliderComponent& Sphere = Registry.get<SSphereColliderComponent>(Entity);
        const STransformComponent& Transform = Registry.get<STransformComponent>(Entity);
        
        PDI->DrawSphere(Transform.GetLocation() * Sphere.Offset, Sphere.Radius * Transform.MaxScale(), FColor::Green, 12, 1.5f, true, 0.0f);
    }

    CStruct* CComponentVisualizer_BoxCollider::GetSupportedComponentType() const
    {
        return SBoxColliderComponent::StaticStruct();
    }

    void CComponentVisualizer_BoxCollider::Draw(IPrimitiveDrawInterface* PDI, entt::registry& Registry, entt::entity Entity)
    {
        const SBoxColliderComponent& Box = Registry.get<SBoxColliderComponent>(Entity);
        const STransformComponent& Transform = Registry.get<STransformComponent>(Entity);
        
        PDI->DrawBox(Transform.GetLocation() + Box.Offset, Box.HalfExtent * Transform.GetScale(), Transform.GetRotation(), FColor::Green, 1.5f, true, 0.0f);
    }

    CStruct* CComponentVisualizer_CharacterPhysics::GetSupportedComponentType() const
    {
        return SCharacterPhysicsComponent::StaticStruct();
    }

    void CComponentVisualizer_CharacterPhysics::Draw(IPrimitiveDrawInterface* PDI, entt::registry& Registry, entt::entity Entity)
    {
        const SCharacterPhysicsComponent& Character = Registry.get<SCharacterPhysicsComponent>(Entity);
        const STransformComponent& Transform = Registry.get<STransformComponent>(Entity);
    
        glm::vec3 Location = Transform.GetLocation();
        glm::vec3 Start = Location - glm::vec3(0, Character.HalfHeight, 0);
        glm::vec3 End = Location + glm::vec3(0, Character.HalfHeight, 0);
    
        PDI->DrawCapsule(Start, End, Character.Radius, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), 12, 2.0f, true, 0.0f);
    }
}
