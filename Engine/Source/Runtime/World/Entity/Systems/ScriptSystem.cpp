#include "pch.h"
#include "ScriptSystem.h"
#include "World/World.h"
#include "world/entity/components/entitytags.h"
#include "World/Entity/Components/ScriptComponent.h"


namespace Lumina
{
    void SScriptSystem::Update(const FSystemContext& Context) noexcept
    {
        LUMINA_PROFILE_SCOPE(); 
    
        auto View = Context.CreateView<SScriptComponent>(entt::exclude<SDisabledTag>);
        View.each([&](entt::entity Entity, SScriptComponent& ScriptComponent)
        {
            if (const TSharedPtr<Lua::FScript>& Script = ScriptComponent.Script)
            {
                if ((Context.GetWorldType() == EWorldType::Editor) == ScriptComponent.bRunInEditor)
                {
                    if (ScriptComponent.UpdateFunc.IsValid())
                    {
                        const float DeltaTime = static_cast<float>(Context.GetDeltaTime());
                    
                        if (ScriptComponent.TickRate <= 0.0f)
                        {
                            ScriptComponent.UpdateFunc(Script->Reference, DeltaTime);
                        }
                        else
                        {
                            ScriptComponent.AccumulatedTime += DeltaTime;
                            if (ScriptComponent.AccumulatedTime >= ScriptComponent.TickRate)
                            {
                                ScriptComponent.UpdateFunc(Script->Reference, ScriptComponent.AccumulatedTime);
                                ScriptComponent.AccumulatedTime = 0.0f;
                            }
                        }
                    }
                }
            }
        });
    }
}
