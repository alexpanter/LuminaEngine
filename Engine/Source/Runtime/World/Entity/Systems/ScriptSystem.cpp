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
        View.each([&](entt::entity, SScriptComponent& ScriptComponent)
        {
            if (const TSharedPtr<Lua::FScript>& Script = ScriptComponent.Script)
            {
                if ((Context.GetWorldType() == EWorldType::Editor) == ScriptComponent.bRunInEditor)
                {
                    if (ScriptComponent.UpdateFunc.IsValid())
                    {
                        ScriptComponent.UpdateFunc(Script->Reference, Context.GetDeltaTime());
                    }
                }
            }
        });
    }
}
