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
        View.each([&](entt::entity Entity, const SScriptComponent& ScriptComponent)
        {
            if (const TSharedPtr<Scripting::FLuaScript>& Script = ScriptComponent.Script)
            {
                if (!Script->ScriptTable.valid())
                {
                    return;
                }
            
                Script->Environment["Entity"] = Entity;
                Script->Environment["Context"] = std::ref(Context);
                
                if (sol::optional<sol::function> BeginPlayFunc = Script->ScriptTable["Update"])
                {
                    sol::protected_function_result Result = (*BeginPlayFunc)(Script->ScriptTable, Context.GetDeltaTime());
                    if (!Result.valid())
                    {
                        sol::error Error = Result;
                        LOG_ERROR("Script Error: {} - {}", Script->Path, Error.what());
                    }
                }
            }
        });
    }
}
